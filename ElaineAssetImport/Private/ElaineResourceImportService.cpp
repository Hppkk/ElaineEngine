#include "ElainePrecompiledHeader.h"
#include "ElaineResourceImportService.h"
#include "ElaineRoot.h"
#include "ElaineFileMonitor.h"
#include "Resource/ElaineResourceLoader.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "TaskGraph/ElaineWorker.h"
#include <fstream>

namespace Elaine
{
    namespace fs = std::filesystem;
    namespace
    {
        std::string PathKey(std::string Path)
        {
#ifdef _WIN32
            std::transform(Path.begin(), Path.end(), Path.begin(), [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
#endif
            return Path;
        }
        fs::path Checked(const fs::path& Root, const std::string& Path)
        {
            auto File = fs::weakly_canonical(Root / CanonicalResourcePath(Path));
            auto Relative = File.lexically_relative(Root);
            if (Relative.empty() || Relative.is_absolute() || *Relative.begin() == ".." || File.extension() != ".asset")
                throw std::runtime_error("Import destination must be an .asset inside the asset root: " + Path);
            return File;
        }
        std::vector<uint8_t> Bytes(const fs::path& File)
        {
            std::ifstream Stream(File, std::ios::binary | std::ios::ate);
            if (!Stream) throw std::runtime_error("Cannot read import file: " + File.string());
            auto Size = Stream.tellg(); if (Size < 0) throw std::runtime_error("Invalid import file size");
            std::vector<uint8_t> Result(static_cast<size_t>(Size)); Stream.seekg(0);
            if (!Result.empty() && !Stream.read(reinterpret_cast<char*>(Result.data()), Size)) throw std::runtime_error("Truncated import file");
            return Result;
        }
        JsonCpp MergeImported(const JsonCpp& Current, const JsonCpp& Base, const JsonCpp& Incoming)
        {
            if (Current == Base) return Incoming;
            if (!Current.is_object() || !Base.is_object() || !Incoming.is_object()) return Current;
            auto Result = Current;
            for (const auto& Entry : Incoming.items())
            {
                const auto& Key = Entry.key();
                if (!Current.contains(Key)) { if (!Base.contains(Key)) Result[Key] = Entry.value(); }
                else if (Base.contains(Key)) Result[Key] = MergeImported(Current.at(Key), Base.at(Key), Entry.value());
            }
            for (const auto& Entry : Base.items())
                if (!Incoming.contains(Entry.key()) && Current.contains(Entry.key()) && Current.at(Entry.key()) == Entry.value()) Result.erase(Entry.key());
            return Result;
        }
        TaskGraph::GraphTaskPtr Internal(std::function<void()> Work, NamedThread Thread)
        {
            auto Task = TaskGraph::TaskGraph::instance()->CreateTask(std::move(Work), Thread);
            Task->SetCategory(TaskGraph::TaskCategory::ResourceInternal); return Task;
        }
        void Wait(const TaskGraph::TaskCompletionEventPtr& Event)
        {
            auto* Graph = TaskGraph::TaskGraph::instance();
            auto* Worker = Graph->GetWorker(NamedThread::GameThread);
            Event->Subscribe([Worker](auto) { Worker->Wake(); });
            while (!Event->IsComplete()) { Graph->ExecuteResourceTasks(); if (!Event->IsComplete()) Graph->WaitForResourceTasks(Event); }
        }
    }

    ImportedAsset ResourceImportContext::MakeAsset(const std::string& Key, const std::string& Type, const std::string& Path) const
    {
        ImportedAsset Asset; Asset.SourceKey = Key;
        auto PreviousRecord = Previous.find(Key);
        if (PreviousRecord != Previous.end())
        {
            Asset.Descriptor = PreviousRecord->second;
            if (Asset.Descriptor.Type != Type) throw std::runtime_error("Imported output type changed: " + Key);
        }
        else
        {
            Asset.Descriptor.Id = ResourceId::New(); Asset.Descriptor.Type = Type;
            Asset.Descriptor.Path = Path; Asset.Descriptor.DisplayName = fs::path(Path).stem().string();
        }
        if (Key == "primary") Asset.Descriptor.Path = Request.Destination;
        return Asset;
    }

    struct ResourceImportService::Operation
    {
        ResourceImportContext Context;
        ResourceImportOutput Output;
        ResourceImportResult Result;
        fs::path Stage;
        std::vector<std::string> Paths, Watched;
        std::map<std::string, fs::path> StagedFiles;
        std::vector<ResourceId> GateOrder;
        TaskGraph::TaskCompletionEventPtr Gate = std::make_shared<TaskGraph::TaskCompletionEvent>();
        std::map<std::string, std::vector<uint8_t>> Backups;
        std::map<std::string, std::vector<uint8_t>> Expected;
        std::vector<std::string> Written;
        bool Claimed = false, Reserved = false, Gated = false, RegistryReserved = false, Published = false, RecoveryNeeded = false;
        void Rollback() noexcept
        {
            for (auto It = Written.rbegin(); It != Written.rend(); ++It)
            {
                try
                {
                    if (auto Old = Backups.find(*It); Old != Backups.end()) AssetFile::AtomicWrite(Checked(Context.Root, *It), Old->second);
                    else fs::remove(Checked(Context.Root, *It));
                }
                catch (const std::exception& Error)
                {
                    RecoveryNeeded = true;
                    Result.Error += "\nCannot restore " + *It + ": " + Error.what();
                }
            }
            Written.clear();
            if (RecoveryNeeded) Result.Error += "\nRecovery files retained at: " + Stage.string();
        }
    };

    ResourceImportService::ResourceImportService(fs::path Root, ResourceRegistry* Registry)
        : mRoot(fs::weakly_canonical(Root)), mRegistry(Registry)
    { RegisterImporter(std::make_unique<TextureImporter>()); RegisterImporter(std::make_unique<MeshImporter>()); }
    ResourceImportService::~ResourceImportService() { Shutdown(); }
    ResourceImportService& ResourceImportService::Get()
    { static ResourceImportService Service(Root::instance()->GetResourcePath(), &ResourceRegistry::Get()); return Service; }
    const ResourceImporter& ResourceImportService::Find(const std::string& Name) const
    {
        auto It = mImporters.find(Name);
        if (It == mImporters.end()) throw std::runtime_error("No resource importer registered: " + Name);
        return *It->second;
    }
    void ResourceImportService::RegisterImporter(std::unique_ptr<ResourceImporter> Importer)
    {
        std::lock_guard Lock(mMutex);
        if (!mJobs.empty() || !mRequests.empty() || mStopping) throw std::logic_error("Register importers before dispatching imports");
        if (!Importer) throw std::invalid_argument("Null resource importer");
        const std::string Name = Importer->GetName();
        if (mImporters.contains(Name)) throw std::runtime_error("Duplicate importer: " + Name);
        mImporters.emplace(Name, std::move(Importer));
    }
    std::vector<std::string> ResourceImportService::GetSupportedExtensions(const std::string& Name) const
    { return Find(Name).GetSupportedExtensions(); }
    JsonCpp ResourceImportService::GetSettingsDescription(const std::string& Name) const
    { return Find(Name).GetSettingsDescription(); }
    std::shared_ptr<ResourceImportService::Operation> ResourceImportService::Prepare(ResourceImportRequest Request)
    {
        { std::lock_guard Lock(mMutex); if (mStopping) throw std::runtime_error("Import service is stopping"); }
        Request.Destination = Checked(mRoot, Request.Destination).lexically_relative(mRoot).generic_string();
        if (Request.Settings) Request.Settings = Request.Settings->Clone();
        for (auto& Source : Request.Sources) Source = fs::absolute(Source).lexically_normal();
        auto Op = std::make_shared<Operation>(); Op->Context.Root = mRoot; Op->Context.Request = std::move(Request);
        { std::lock_guard Lock(mMutex);
          if (!mRequests.insert(PathKey(Op->Context.Request.Destination)).second) throw std::runtime_error("An import is already pending for this destination");
          Op->Claimed = true; }
        return Op;
    }
    void ResourceImportService::Convert(Operation& Op)
    {
        auto& Context = Op.Context; auto& Request = Context.Request;
        if (Request.Reimport)
        {
            Op.Expected[Request.Destination] = Bytes(mRoot / Request.Destination);
            const auto Previous = AssetFile::ReadDescriptor(mRoot / Request.Destination);
            const auto Record = Previous.ExtraFields.value("import", JsonCpp::object());
            Request.Importer = Record.value("importer", Previous.Type);
            if (Record.contains("sourceKey") && Record.at("sourceKey") != "primary") throw std::runtime_error("Reimport the owning primary asset");
            if (!Request.Settings)
            {
                JsonCpp Settings = Record.value("settings", JsonCpp::object());
                if (Previous.Type == "Texture" && !Record.contains("settings"))
                { auto Data = AssetFile::ReadSection(mRoot / Request.Destination, "import"); Settings = JsonCpp::parse(Data.begin(), Data.end()).at("settings"); }
                Request.Settings = Find(Request.Importer).ReadSettings(Settings);
            }
            if (Previous.Type == "Mesh" && Request.Sources.empty())
                for (const auto& Source : Record.value("sources", std::vector<std::string>{})) Request.Sources.emplace_back(Source);
            if (Previous.Type == "Mesh" && Request.Sources.empty()) throw std::runtime_error("No model source is recorded; this upgraded Mesh cannot be reimported");
            Context.Previous["primary"] = Previous;
            for (const auto& Entry : Record.value("outputs", JsonCpp::array()))
            {
                auto Ref = ResourceReference::FromJson(Entry.at("reference"));
                auto Path = Ref.Path;
                if (mRegistry) { try { Path = mRegistry->Find(Ref.Id).Path; } catch (...) {} }
                if (!Entry.value("active", true) && !fs::exists(Checked(mRoot, Path))) continue;
                Op.Expected[Path] = Bytes(Checked(mRoot, Path));
                auto Descriptor = AssetFile::ReadDescriptor(Checked(mRoot, Path)); Descriptor.Path = Path;
                if (Descriptor.Id != Ref.Id) throw std::runtime_error("Reimport output identity changed: " + Path);
                Context.Previous[Entry.at("key").get<std::string>()] = Descriptor;
            }
            Context.Previous["primary"].Path = Request.Destination;
        }
        const auto& Importer = Find(Request.Importer);
        if (!Request.Settings) Request.Settings = Importer.ReadSettings(JsonCpp::object());
        auto Extensions = Importer.GetSupportedExtensions();
        for (const auto& Source : Request.Sources)
        {
            auto Extension = Source.extension().string();
            std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
            if (std::find(Extensions.begin(), Extensions.end(), Extension) == Extensions.end()) throw std::runtime_error("Unsupported import format: " + Source.string());
        }
        Op.Output = Importer.Build(Context);
        if (Op.Output.Assets.empty() || Op.Output.Assets.front().SourceKey != "primary") throw std::runtime_error("Importer did not return a primary asset");
        JsonCpp Outputs = JsonCpp::array(); std::set<std::string> Keys, Paths; std::set<ResourceId> IDs;
        for (auto& Asset : Op.Output.Assets)
        {
            const auto& D = Asset.Descriptor;
            Checked(mRoot, D.Path);
            if (!D.Id.IsValid() || !IDs.insert(D.Id).second || !Keys.insert(Asset.SourceKey).second || !Paths.insert(PathKey(D.Path)).second) throw std::runtime_error("Duplicate imported output");
            Outputs.push_back({{"key", Asset.SourceKey}, {"reference", ResourceReference{D.Id, D.Path}.ToJson()}});
        }
        // Keep inactive source identities so a temporarily removed source material can return with its old ID.
        for (const auto& [Key, Descriptor] : Context.Previous) if (!Keys.contains(Key))
            Outputs.push_back({{"key", Key}, {"reference", ResourceReference{Descriptor.Id, Descriptor.Path}.ToJson()}, {"active", false}});
        std::vector<std::string> Sources;
        for (const auto& Source : Request.Sources) Sources.push_back(Source.generic_string());
        if (Sources.empty() && Request.Reimport && Context.Previous.contains("primary"))
            Sources = Context.Previous.at("primary").ExtraFields.value("import", JsonCpp::object()).value("sources", std::vector<std::string>{});
        for (auto& Asset : Op.Output.Assets)
        {
            auto Incoming = Asset.Data;
            auto Import = Asset.Descriptor.ExtraFields.value("import", JsonCpp::object());
            if (!Incoming.is_null() && Request.Reimport)
            {
                auto Previous = Context.Previous.find(Asset.SourceKey);
                if (Previous != Context.Previous.end())
                {
                    const auto Saved = AssetFile::ReadJson(mRoot / Previous->second.Path);
                    Asset.Data = MergeImported(Saved, Import.value("baseline", JsonCpp::object()), Incoming);
                }
            }
            Import["importer"] = Request.Importer; Import["settings"] = Request.Settings->ToJson(); Import["sources"] = Sources;
            Import["sourceKey"] = Asset.SourceKey;
            if (!Incoming.is_null()) Import["baseline"] = std::move(Incoming);
            if (Asset.SourceKey == "primary") Import["outputs"] = Outputs;
            Asset.Descriptor.ExtraFields["import"] = std::move(Import);
        }
        for (const auto& [Key, D] : Context.Previous) if (!Keys.contains(Key)) Op.Result.UnusedAssets.push_back(D);
        Op.Result.Warnings = Op.Output.Warnings;
        // Staging is outside Contents, so the registry and watcher cannot observe partial output.
        Op.Stage = mRoot.parent_path() / "Intermediate" / "AssetImport" / ResourceId::New().ToString();
        for (auto& Asset : Op.Output.Assets)
        {
            auto File = Op.Stage / (Asset.Descriptor.Id.ToString() + ".asset");
            Op.StagedFiles[Asset.Descriptor.Path] = File;
            if (Asset.Data.is_null()) AssetFile::Write(File, Asset.Descriptor, Asset.Sections);
            else AssetFile::WriteJson(File, Asset.Descriptor, Asset.Data);
            auto Saved = AssetFile::ReadDescriptor(File); Saved.Path = Asset.Descriptor.Path;
            Asset.Descriptor = Saved; Op.Result.Assets.push_back(Saved);
            Op.Paths.push_back(Saved.Path);
        }
        Op.Result.Primary = Op.Result.Assets.front();
    }
    void ResourceImportService::Reserve(Operation& Op)
    {
        std::lock_guard Lock(mMutex);
        if (mStopping) throw std::runtime_error("Import cancelled during shutdown");
        for (const auto& Asset : Op.Output.Assets)
        {
            const auto& Path = Asset.Descriptor.Path;
            if (mReserved.contains(PathKey(Path))) throw std::runtime_error("Import destination is busy: " + Path);
            if (fs::exists(Checked(mRoot, Path)))
            {
                auto Previous = Op.Context.Previous.find(Asset.SourceKey);
                if (!Op.Context.Request.Reimport || Previous == Op.Context.Previous.end() ||
                    AssetFile::ReadDescriptor(mRoot / Path).Id != Previous->second.Id)
                    throw std::runtime_error("Import destination already exists: " + Path);
            }
        }
        for (const auto& Path : Op.Paths) mReserved.insert(PathKey(Path));
        Op.Reserved = true;
        if (mRegistry)
        {
            mRegistry->ReserveImportPaths(Op.Paths); Op.RegistryReserved = true;
            // Gate parents before their dependencies. An already running parent must be able to
            // start its old children before we prevent new reads of those children.
            std::map<ResourceId, size_t> Incoming;
            std::map<ResourceId, std::set<ResourceId>> Edges;
            for (const auto& Asset : Op.Output.Assets) Incoming[Asset.Descriptor.Id] = 0;
            auto Add = [&](const ResourceDescriptor& D) {
                for (const auto& Dependency : D.Dependencies)
                    if (Dependency.Kind == ResourceDependencyKind::Hard && Incoming.contains(Dependency.Reference.Id) &&
                        Edges[D.Id].insert(Dependency.Reference.Id).second) ++Incoming[Dependency.Reference.Id];
            };
            for (const auto& Asset : Op.Output.Assets)
            {
                Add(Asset.Descriptor);
                if (auto Old = Op.Context.Previous.find(Asset.SourceKey); Old != Op.Context.Previous.end()) Add(Old->second);
            }
            std::vector<ResourceId> Ready;
            for (const auto& [Id, Count] : Incoming) if (!Count) Ready.push_back(Id);
            for (size_t I=0; I<Ready.size(); ++I)
            {
                Op.GateOrder.push_back(Ready[I]);
                for (const auto& Child : Edges[Ready[I]]) if (--Incoming[Child] == 0) Ready.push_back(Child);
            }
            if (Op.GateOrder.size() != Incoming.size()) throw std::runtime_error("Cyclic imported output dependencies");
            if (auto* Monitor = FileMonitor::instance())
            { for (const auto& Path : Op.Paths) Op.Watched.push_back((mRoot / Path).string()); Monitor->DeferChanges(Op.Watched, true); }
        }
    }
    void ResourceImportService::ScheduleCommit(const std::shared_ptr<Operation>& Op,
        const TaskGraph::TaskCompletionEventPtr& Submitted, size_t Index)
    {
        try
        {
            for (; Index < Op->GateOrder.size(); ++Index)
            {
                auto Waits = ResourceLoader::instance()->GateResources({Op->GateOrder[Index]}, Op->Gate);
                Op->Gated = true;
                if (std::all_of(Waits.begin(), Waits.end(), [](const auto& Event) { return Event->IsComplete(); })) continue;
                auto Next = Internal([this, Op, Submitted, Index] { ScheduleCommit(Op, Submitted, Index + 1); }, NamedThread::GameThread);
                for (const auto& Event : Waits) Next->Dependency(Event);
                Next->SetContinuationPolicy(TaskGraph::TaskContinuationPolicy::Always);
                auto Ready = Next->GetCompletionEvent();
                Ready->Subscribe([Ready, Submitted](auto State) {
                    if (State == TaskGraph::TaskState::Failed) Submitted->Fail(Ready->GetException());
                    else if (State == TaskGraph::TaskState::Cancelled) Submitted->Cancel();
                });
                TaskGraph::TaskGraph::instance()->Dispatch(Next);
                return;
            }
            auto Save = Internal([this, Op] { Commit(*Op); }, NamedThread::AnyThread);
            auto Event = Save->GetCompletionEvent();
            Event->Subscribe([Event, Submitted](auto State) {
                if (State == TaskGraph::TaskState::Succeeded) Submitted->Succeed();
                else if (auto Error = Event->GetException()) Submitted->Fail(Error);
                else Submitted->Cancel();
            });
            TaskGraph::TaskGraph::instance()->Dispatch(Save);
        }
        catch (...) { Submitted->Fail(std::current_exception()); }
    }
    void ResourceImportService::Commit(Operation& Op)
    {
        try
        {
            for (const auto& Path : Op.Paths)
            {
                if (fs::exists(Checked(mRoot, Path)))
                {
                    auto Old = Bytes(mRoot / Path);
                    auto Expected = Op.Expected.find(Path);
                    if (Expected == Op.Expected.end() || Expected->second != Old)
                        throw std::runtime_error("Import output changed during conversion; retry the import: " + Path);
                    AssetFile::AtomicWrite(Op.Stage / "backup" / (std::to_string(Op.Backups.size()) + ".asset"), Old);
                    Op.Backups[Path] = std::move(Old);
                }
                else if (Op.Context.Request.Reimport && std::any_of(Op.Context.Previous.begin(), Op.Context.Previous.end(),
                    [&](const auto& Entry) { return Entry.second.Path == Path; })) throw std::runtime_error("Reimport output was removed: " + Path);
                AssetFile::AtomicWrite(Checked(mRoot, Path), Bytes(Op.StagedFiles.at(Path))); Op.Written.push_back(Path);
            }
        }
        catch (...) { Op.Rollback(); throw; }
    }
    void ResourceImportService::Publish(Operation& Op)
    {
        if (mRegistry) mRegistry->PublishImported(Op.Result.Assets);
        Op.Published = true;
    }
    void ResourceImportService::Release(Operation& Op)
    {
        if (!Op.Published && !Op.Written.empty()) Op.Rollback();
        if (Op.Gated) { ResourceLoader::instance()->ReleaseResources(Op.Gate); Op.Gate->Succeed(); Op.Gated = false; }
        if (Op.RegistryReserved) { mRegistry->ReleaseImportPaths(Op.Paths); Op.RegistryReserved = false; }
        if (!Op.Watched.empty()) { FileMonitor::instance()->DeferChanges(Op.Watched, false); Op.Watched.clear(); }
        if (Op.Reserved) { std::lock_guard Lock(mMutex); for (const auto& Path : Op.Paths) mReserved.erase(PathKey(Path)); Op.Reserved = false; }
        if (Op.Claimed) { std::lock_guard Lock(mMutex); mRequests.erase(PathKey(Op.Context.Request.Destination)); Op.Claimed = false; }
        if (!Op.Stage.empty() && !Op.RecoveryNeeded) { std::error_code Error; fs::remove_all(Op.Stage, Error); }
        if (Op.Published && mRegistry && Op.Context.Request.Reimport)
            for (const auto& D : Op.Result.Assets)
                if (auto* Loader = ResourceLoader::instance()) if (auto* Manager = Loader->GetManager(static_cast<ResourceType>(ResourceTypeFromName(D.Type))))
                    Manager->ReloadIfFileChanged((mRoot / D.Path).string());
    }
    TaskGraph::TaskCompletionEventPtr ResourceImportService::Import(ResourceImportRequest Request, ResourceImportCompletion Completion)
    {
        auto Op = Prepare(std::move(Request)); auto* Graph = TaskGraph::TaskGraph::instance();
        if (!Graph) { Release(*Op); throw std::logic_error("Async import requires TaskGraph; tools can use ImportBlocking"); }
        auto Submitted = std::make_shared<TaskGraph::TaskCompletionEvent>();
        auto Build = Internal([this, Op] { Convert(*Op); }, NamedThread::AnyThread);
        auto ReserveTask = Internal([this, Op, Submitted] {
            Reserve(*Op);
            ScheduleCommit(Op, Submitted);
        }, NamedThread::GameThread);
        ReserveTask->Dependency(Build);
        auto Reserved = ReserveTask->GetCompletionEvent(); auto Built = Build->GetCompletionEvent();
        Reserved->Subscribe([Submitted, Reserved, Built](auto State) {
            if (State != TaskGraph::TaskState::Succeeded)
            {
                auto Error = Built->GetException(); if (!Error) Error = Reserved->GetException();
                if (Error) Submitted->Fail(Error); else Submitted->Cancel();
            }
        });
        auto Done = Internal([this, Op, Submitted] {
            try
            {
                Submitted->RethrowIfFailed();
                if (Submitted->GetState() != TaskGraph::TaskState::Succeeded) throw std::runtime_error("Import cancelled");
                Publish(*Op); Release(*Op);
            }
            catch (const std::exception& Error) { Op->Result.Error = std::string(Error.what()) + Op->Result.Error; Release(*Op); throw; }
        }, NamedThread::GameThread);
        Done->Dependency(Submitted); Done->SetContinuationPolicy(TaskGraph::TaskContinuationPolicy::Always);
        if (Completion)
        {
            auto Notify = Graph->CreateTask([Op, Completion, Active=mCallbacksActive] { if (Active->load()) Completion(Op->Result); }, NamedThread::GameThread);
            Notify->SetCategory(TaskGraph::TaskCategory::ResourceCallback);
            Notify->SetContinuationPolicy(TaskGraph::TaskContinuationPolicy::Always); Notify->Dependency(Done); Graph->Dispatch(Notify);
        }
        { std::lock_guard Lock(mMutex); std::erase_if(mJobs, [](auto& Event) { return Event->IsComplete(); }); mJobs.push_back(Done->GetCompletionEvent()); }
        Graph->Dispatch(Done); Graph->Dispatch(ReserveTask); Graph->Dispatch(Build);
        return Done->GetCompletionEvent();
    }
    TaskGraph::TaskCompletionEventPtr ResourceImportService::ImportTexture(const std::vector<fs::path>& Sources, const std::string& Destination,
        const TextureImportSettings& Settings, ResourceImportCompletion Completion)
    { return Import({"Texture", Sources, Destination, Settings.Clone(), false}, std::move(Completion)); }
    TaskGraph::TaskCompletionEventPtr ResourceImportService::ImportMesh(const fs::path& Source, const std::string& Destination,
        const MeshImportSettings& Settings, ResourceImportCompletion Completion)
    { return Import({"Mesh", {Source}, Destination, Settings.Clone(), false}, std::move(Completion)); }
    TaskGraph::TaskCompletionEventPtr ResourceImportService::Reimport(const std::string& Path, ResourceImportCompletion Completion)
    { return Import({{}, {}, Path, {}, true}, std::move(Completion)); }
    ResourceImportResult ResourceImportService::ImportBlocking(ResourceImportRequest Request)
    {
        if (mRegistry) throw std::logic_error("Blocking import is an offline tool API; use async imports with a live registry");
        auto Op = Prepare(std::move(Request));
        try { Convert(*Op); Reserve(*Op); Commit(*Op); Publish(*Op); Release(*Op); }
        catch (const std::exception& Error) { Op->Result.Error = std::string(Error.what()) + Op->Result.Error; Release(*Op); }
        return Op->Result;
    }
    ResourceImportResult ResourceImportService::ImportTextureBlocking(const std::vector<fs::path>& Sources, const std::string& Destination, const TextureImportSettings& Settings)
    { return ImportBlocking({"Texture", Sources, Destination, Settings.Clone(), false}); }
    ResourceImportResult ResourceImportService::ReimportBlocking(const std::string& Path, std::shared_ptr<const ResourceImportSettings> Settings)
    { return ImportBlocking({{}, {}, Path, std::move(Settings), true}); }
    void ResourceImportService::Shutdown()
    {
        std::vector<TaskGraph::TaskCompletionEventPtr> Jobs;
        { std::lock_guard Lock(mMutex); if (mStopping) return; mStopping = true; mCallbacksActive->store(false); Jobs.swap(mJobs); }
        for (const auto& Job : Jobs) if (!Job->IsComplete()) Wait(Job);
    }
}
