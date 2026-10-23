#include "ElainePrecompiledHeader.h"
#include "Resource/ElaineResourceRegistry.h"
#include "Resource/ElaineResourceLoader.h"
#include "Resource/ElaineResourceManager.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "TaskGraph/ElaineWorker.h"
#include <fstream>
#include <regex>
#include <set>

namespace Elaine
{
    namespace
    {
        std::filesystem::path CheckedPath(const std::filesystem::path& Root, const std::string& Name)
        {
            const auto Target = std::filesystem::weakly_canonical(Root / CanonicalResourcePath(Name));
            const auto Relative = Target.lexically_relative(std::filesystem::weakly_canonical(Root));
            if (Relative.empty() || Relative == "." || Relative.is_absolute() || *Relative.begin() == "..")
                throw std::runtime_error("Resource operation is outside Contents: " + Name);
            return Target;
        }
        bool IsAsset(const std::filesystem::path& Path) { return Path.extension() == ".asset" || Path.extension() == ".shader"; }
        TaskGraph::GraphTaskPtr InternalTask(std::function<void()> Work, NamedThread Thread)
        {
            auto Task = TaskGraph::TaskGraph::instance()->CreateTask(std::move(Work), Thread);
            Task->SetCategory(TaskGraph::TaskCategory::ResourceInternal); return Task;
        }
        void WaitInternal(const TaskGraph::TaskCompletionEventPtr& Event)
        {
            if (!Event) return;
            auto* Graph = TaskGraph::TaskGraph::instance();
            auto* Worker = Graph->GetWorker(NamedThread::GameThread);
            Event->Subscribe([Worker](auto) { Worker->Wake(); });
            while (!Event->IsComplete()) { Graph->ExecuteResourceTasks(); if (!Event->IsComplete()) Graph->WaitForResourceTasks(Event); }
            Event->RethrowIfFailed();
        }
        std::string Lower(std::string Text)
        { std::transform(Text.begin(), Text.end(), Text.begin(), [](unsigned char C) { return static_cast<char>(std::tolower(C)); }); return Text; }
    }
    ResourceRegistry& ResourceRegistry::Get() { static ResourceRegistry Registry; return Registry; }
    std::filesystem::path ResourceRegistry::RootPath() const { std::lock_guard Lock(mMutex); return mRoot; }
    uint64_t ResourceRegistry::Revision() const { std::lock_guard Lock(mMutex); return mRevision; }
    ResourceDescriptor ResourceRegistry::Read(const std::filesystem::path& Root, const std::string& Path)
    {
        ResourceDescriptor Record;
        try
        {
            Record = AssetFile::ReadDescriptor(Root / Path); Record.Path = Path;
            if (Record.Type == "Shader")
            {
                // Source-file graph only; browsing never invokes the shader compiler.
                std::set<std::string> Seen;
                std::function<void(std::string)> Visit = [&](std::string Source) {
                    Source = CanonicalResourcePath(Source);
                    if (!Seen.insert(Source).second) return;
                    std::ifstream File(Root / Source);
                    if (!File) { Record.Tags["sourceWarning"] = "Missing shader source/include: " + Source; return; }
                    std::string Line;
                    const std::regex Include(R"inc(^\s*#\s*include\s*["<]([^">]+)[">])inc");
                    while (std::getline(File, Line))
                    {
                        std::smatch Match;
                        if (!std::regex_search(Line, Match, Include)) continue;
                        const std::string Requested = Match[1].str();
                        if (Requested.starts_with("Generated/")) continue;
                        auto Candidate = (std::filesystem::path(Source).parent_path() / Requested).lexically_normal();
                        if (!std::filesystem::exists(Root / Candidate)) Candidate = std::filesystem::path("shader/vulkan") / Requested;
                        Visit(Candidate.generic_string());
                    }
                };
                for (const auto& Source : Record.SourceFiles) Visit(Source);
                Record.SourceFiles.assign(Seen.begin(), Seen.end());
            }
        }
        catch (const std::exception& Error) { Record.Path = Path; Record.Error = Error.what(); }
        return Record;
    }
    TaskGraph::TaskCompletionEventPtr ResourceRegistry::Start(const std::filesystem::path& Root)
    {
        uint64_t Epoch, Ticket;
        { std::lock_guard Lock(mMutex); mRoot = std::filesystem::weakly_canonical(Root); mStopping = false;
          Epoch = ++mEpoch; Ticket = ++mTicket; mRecords.clear(); mIDs.clear(); mKnownPaths.clear(); mReferencers.clear(); mTickets.clear(); }
        auto Records = std::make_shared<ResourceRegistry::Records>();
        auto Scan = InternalTask([Root = RootPath(), Records] {
            if (!std::filesystem::is_directory(Root)) throw std::runtime_error("Asset root is not a directory: " + Root.string());
            std::error_code Error;
            auto It = std::filesystem::recursive_directory_iterator(Root, Error);
            if (Error) throw std::runtime_error("Cannot scan asset directory: " + Error.message());
            for (std::filesystem::recursive_directory_iterator End; It != End; It.increment(Error))
            {
                if (Error) throw std::runtime_error("Cannot scan asset directory: " + Error.message());
                if (It->is_regular_file() && IsAsset(It->path()))
                { const auto Path = std::filesystem::relative(It->path(), Root).generic_string(); (*Records)[Path] = Read(Root, Path); }
            }
            if (Error) throw std::runtime_error("Cannot scan asset directory: " + Error.message());
        }, NamedThread::AnyThread);
        auto Done = InternalTask([this, Records, Epoch, Ticket, Event = Scan->GetCompletionEvent()] {
            Event->RethrowIfFailed();
            Publish(std::move(*Records), Epoch, Ticket, true);
        }, NamedThread::GameThread);
        Done->SetContinuationPolicy(TaskGraph::TaskContinuationPolicy::Always);
        Done->Dependency(Scan);
        { std::lock_guard Lock(mMutex); mReady = Done->GetCompletionEvent(); mJobs.push_back(mReady); }
        auto* Graph = TaskGraph::TaskGraph::instance(); Graph->Dispatch(Done); Graph->Dispatch(Scan);
        return Done->GetCompletionEvent();
    }
    void ResourceRegistry::WaitForStartup() { TaskGraph::TaskCompletionEventPtr Ready; { std::lock_guard Lock(mMutex); Ready = mReady; } WaitInternal(Ready); }
    void ResourceRegistry::Stop()
    {
        std::vector<TaskGraph::TaskCompletionEventPtr> Jobs;
        { std::lock_guard Lock(mMutex); mStopping = true; ++mEpoch; Jobs.swap(mJobs); for (auto& Entry : mObservers) Entry.first->store(false); mObservers.clear(); }
        for (const auto& Job : Jobs) { try { WaitInternal(Job); } catch (...) {} }
    }
    TaskGraph::TaskCompletionEventPtr ResourceRegistry::Refresh(const std::vector<std::string>& Paths)
    {
        uint64_t Epoch, Ticket; std::filesystem::path Root;
        auto Records = std::make_shared<ResourceRegistry::Records>();
        std::set<std::string> Targets;
        { std::lock_guard Lock(mMutex);
          if (mStopping) { auto Event = std::make_shared<TaskGraph::TaskCompletionEvent>(); Event->Cancel(); return Event; }
          Epoch = mEpoch; Ticket = ++mTicket; Root = mRoot;
          for (auto Path : Paths)
          {
              if (std::filesystem::path(Path).is_absolute()) Path = std::filesystem::path(Path).lexically_relative(Root).generic_string();
              Path = CanonicalResourcePath(Path);
              if (IsAsset(Path)) Targets.insert(Path);
              for (const auto& Pair : mRecords)
                  if (std::find(Pair.second.SourceFiles.begin(), Pair.second.SourceFiles.end(), Path) != Pair.second.SourceFiles.end()) Targets.insert(Pair.first);
          }
          std::erase_if(Targets, [this](const auto& Target) { return mImportPaths.contains(Target); });
          for (const auto& Target : Targets) mTickets[Target] = Ticket;
        }
        auto Scan = InternalTask([Root, Targets, Records] {
            for (const auto& Path : Targets)
                if (std::filesystem::is_regular_file(Root / Path)) (*Records)[Path] = Read(Root, Path);
                else { ResourceDescriptor Missing; Missing.Path = Path; Missing.Error = "Removed"; (*Records)[Path] = std::move(Missing); }
        }, NamedThread::AnyThread);
        auto Done = InternalTask([this, Records, Epoch, Ticket, Event = Scan->GetCompletionEvent()] {
            Event->RethrowIfFailed();
            Publish(std::move(*Records), Epoch, Ticket, false);
        }, NamedThread::GameThread);
        Done->SetContinuationPolicy(TaskGraph::TaskContinuationPolicy::Always);
        Done->Dependency(Scan); auto* Graph = TaskGraph::TaskGraph::instance(); Graph->Dispatch(Done); Graph->Dispatch(Scan);
        { std::lock_guard Lock(mMutex); std::erase_if(mJobs, [](const auto& Job) { return Job->IsComplete(); }); mJobs.push_back(Done->GetCompletionEvent()); }
        return Done->GetCompletionEvent();
    }
    void ResourceRegistry::Rebuild()
    {
        mIDs.clear(); mReferencers.clear(); std::map<ResourceId, std::vector<std::string>> Duplicates;
        for (auto& [Path, Record] : mRecords)
        {
            if (Record.Error.starts_with("Duplicate resource ID")) Record.Error.clear();
            if (Record.Id.IsValid()) Duplicates[Record.Id].push_back(Path);
        }
        for (const auto& [Id, Paths] : Duplicates)
            if (Paths.size() == 1) mIDs[Id] = Paths.front();
            else for (const auto& Path : Paths) mRecords[Path].Error = "Duplicate resource ID: " + Id.ToString();
        for (const auto& [Path, Record] : mRecords)
            for (const auto& Reference : Record.Dependencies) mReferencers[Reference.Reference.Id].push_back(Path);
        for (auto& [Path, Record] : mRecords)
        {
            Record.Tags.erase("missingReferences");
            for (const auto& Reference : Record.Dependencies) if (!mIDs.contains(Reference.Reference.Id)) Record.Tags["missingReferences"].push_back(Reference.ToJson());
        }
    }
    void ResourceRegistry::Publish(Records Records, uint64_t Epoch, uint64_t Ticket, bool Replace)
    {
        std::vector<std::pair<std::shared_ptr<std::atomic_bool>, std::function<void()>>> Observers;
        std::vector<ResourceId> Locations;
        { std::lock_guard Lock(mMutex); if (mStopping || mEpoch != Epoch) return;
          if (Replace)
          {
              for (auto& Pair : Records) if (!mTickets.contains(Pair.first) || mTickets[Pair.first] <= Ticket) mRecords[Pair.first] = std::move(Pair.second);
          }
          else for (auto& [Path, Record] : Records)
          {
              if (mTickets[Path] != Ticket) continue;
              if (Record.Error == "Removed") mRecords.erase(Path); else mRecords[Path] = std::move(Record);
          }
          Rebuild(); ++mRevision;
          for (auto& [Id, OldPath] : mKnownPaths)
              if (!mIDs.contains(Id)) { if (!OldPath.empty()) { Locations.push_back(Id); OldPath.clear(); } }
              else if (mIDs.at(Id) != OldPath) Locations.push_back(Id);
          for (const auto& Pair : mIDs) mKnownPaths[Pair.first] = Pair.second;
          std::erase_if(mObservers, [](const auto& Entry) { return !Entry.first->load(); }); Observers = mObservers;
        }
        for (const auto& Id : Locations) SynchronizeLocation(Id);
        SaveCache();
        for (auto& Entry : Observers) QueueResourceCallback([Entry] { if (Entry.first->load()) Entry.second(); });
    }
    void ResourceRegistry::SaveCache() const
    {
        JsonCpp Json = {{"version", 1}, {"resources", JsonCpp::array()}}; std::filesystem::path Root;
        { std::lock_guard Lock(mMutex); Root = mRoot;
          for (const auto& Pair : mRecords) if (Pair.second.Id.IsValid())
          { auto Node = Pair.second.ToJson(); Node["path"] = Pair.first; Node["error"] = Pair.second.Error; Json["resources"].push_back(Node); } }
        try
        {
            Json["root"] = Root.generic_string(); const auto Text = Json.dump(2);
            const auto Cache = Root.parent_path() / "Intermediate/ResourceRegistry" / (std::to_string(std::hash<std::string>{}(Root.generic_string())) + ".json");
            AssetFile::AtomicWrite(Cache, {Text.begin(), Text.end()});
        }
        catch (const std::exception& Error) { LOG_WARN("Cannot save resource registry: %s", Error.what()); }
    }
    ResourceDescriptor ResourceRegistry::Find(const ResourceId& Id) const
    {
        std::lock_guard Lock(mMutex); auto It = mIDs.find(Id);
        if (It == mIDs.end()) throw std::runtime_error("Missing or ambiguous resource ID: " + Id.ToString());
        const auto& Record = mRecords.at(It->second);
        if (!Record.Error.empty()) throw std::runtime_error(Record.Error);
        return Record;
    }
    ResourceDescriptor ResourceRegistry::FindByPath(const std::string& Path) const
    {
        std::lock_guard Lock(mMutex); const auto Key = CanonicalResourcePath(Path); auto It = mRecords.find(Key);
        if (It == mRecords.end()) It = std::find_if(mRecords.begin(), mRecords.end(), [&](const auto& Pair) { return Lower(Pair.first) == Lower(Key); });
        if (It == mRecords.end()) throw std::runtime_error("Resource is not registered: " + Key);
        if (!It->second.Error.empty()) throw std::runtime_error(It->second.Error);
        return It->second;
    }
    ResourceReference ResourceRegistry::Reference(const std::string& Path) const { auto Record = FindByPath(Path); return {Record.Id, Record.Path}; }
    std::string ResourceRegistry::Resolve(const ResourceReference& Ref, const std::string& Type) const
    { auto Record = Find(Ref.Id); if (!Type.empty() && Record.Type != Type) throw std::runtime_error("Resource type mismatch: " + Record.Path); return Record.Path; }
    std::vector<ResourceDescriptor> ResourceRegistry::Query(const ResourceQuery& Query) const
    {
        std::vector<ResourceDescriptor> Results; std::lock_guard Lock(mMutex);
        for (const auto& [Path, Record] : mRecords)
        {
            const auto Parent = std::filesystem::path(Path).parent_path().generic_string();
            if (!Query.Directory.empty() && (Query.Recursive ? !Path.starts_with(Query.Directory + "/") : Parent != Query.Directory)) continue;
            if (!Query.Recursive && Query.Directory.empty() && !Parent.empty()) continue;
            if (!Query.Type.empty() && Record.Type != Query.Type) continue;
            if (!Query.Search.empty() && Lower(Path + " " + Record.DisplayName).find(Lower(Query.Search)) == std::string::npos) continue;
            if (!Query.Tag.empty() && (!Record.Tags.contains(Query.Tag) || (!Query.Value.empty() && Record.Tags[Query.Tag].dump().find(Query.Value) == std::string::npos))) continue;
            Results.push_back(Record);
        }
        return Results;
    }
    std::vector<ResourceDependency> ResourceRegistry::GetDependencies(const ResourceId& Id) const { return Find(Id).Dependencies; }
    std::vector<ResourceDescriptor> ResourceRegistry::GetReferencers(const ResourceId& Id) const
    {
        std::vector<ResourceDescriptor> Results; std::lock_guard Lock(mMutex); auto It = mReferencers.find(Id);
        if (It != mReferencers.end()) for (const auto& Path : It->second) Results.push_back(mRecords.at(Path)); return Results;
    }
    ResourceSubscription ResourceRegistry::ObserveChanges(std::function<void()> Callback)
    { auto Active = std::make_shared<std::atomic_bool>(true); std::lock_guard Lock(mMutex); mObservers.emplace_back(Active, std::move(Callback)); return ResourceSubscription(Active); }
    void ResourceRegistry::SynchronizeLocation(const ResourceId& Id)
    {
        auto Gate = std::make_shared<TaskGraph::TaskCompletionEvent>();
        std::vector<TaskGraph::TaskCompletionEventPtr> Waits;
        try { Waits = ResourceLoader::instance()->GateResources({Id}, Gate); }
        catch (const std::exception&) { return; } // An explicit operation already owns this identity.
        auto Task = InternalTask([this, Id, Gate] {
            try
            {
                std::string Path;
                { std::lock_guard Lock(mMutex); if (auto It=mIDs.find(Id); It!=mIDs.end()) Path=It->second; }
                if (Path.empty()) ResourceLoader::instance()->UnloadResource(Id);
                else ResourceLoader::instance()->RelocateResource(Id, Path);
                ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed();
            }
            catch (...) { ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed(); throw; }
        }, NamedThread::GameThread);
        Task->SetContinuationPolicy(TaskGraph::TaskContinuationPolicy::Always);
        for (const auto& Event : Waits) Task->Dependency(Event);
        { std::lock_guard Lock(mMutex); mJobs.push_back(Task->GetCompletionEvent()); }
        TaskGraph::TaskGraph::instance()->Dispatch(Task);
    }
    TaskGraph::TaskCompletionEventPtr ResourceRegistry::Move(const std::string& From, const std::string& To)
    {
        const auto Source = CanonicalResourcePath(From), Destination = CanonicalResourcePath(To);
        const auto Root = RootPath(); CheckedPath(Root, Source); CheckedPath(Root, Destination);
        { std::lock_guard Lock(mMutex); for (const auto& Path : mImportPaths)
            if (Path == Source || Path.starts_with(Source + "/") || Path == Destination || Path.starts_with(Destination + "/"))
                throw std::runtime_error("Resource import is still in progress: " + Path); }
        if (Source == Destination || Destination.starts_with(Source + "/")) throw std::runtime_error("Invalid move destination");
        if (IsAsset(Source) && std::filesystem::path(Source).extension() != std::filesystem::path(Destination).extension())
            throw std::runtime_error("Resource extension cannot change");
        if (std::filesystem::exists(Root / Destination)) throw std::runtime_error("Move destination exists");
        std::vector<ResourceId> IDs; std::vector<std::pair<ResourceId, std::string>> Changes;
        for (const auto& Record : Query()) if (Record.Path == Source || Record.Path.starts_with(Source + "/"))
        { IDs.push_back(Record.Id); Changes.emplace_back(Record.Id, Destination + Record.Path.substr(Source.size())); }
        struct ShaderEdit { ResourceId Id; std::string OldPath, NewPath, Original, Updated; };
        std::vector<ShaderEdit> ShaderEdits;
        if (std::filesystem::is_directory(Root / Source))
            for (const auto& Record : Query()) if (Record.Type == "Shader")
            {
                std::ifstream File(Root / Record.Path, std::ios::binary); std::string Original((std::istreambuf_iterator<char>(File)), {});
                auto Json = JsonCpp::parse(Original); auto ShaderSource = CanonicalResourcePath(Json.at("source").get<std::string>());
                if (!ShaderSource.starts_with(Source + "/")) continue;
                Json["source"] = Destination + ShaderSource.substr(Source.size());
                const auto NewPath = Record.Path.starts_with(Source + "/") ? Destination + Record.Path.substr(Source.size()) : Record.Path;
                ShaderEdits.push_back({Record.Id, Record.Path, NewPath, Original, Json.dump(2)});
                if (std::find(IDs.begin(), IDs.end(), Record.Id) == IDs.end()) IDs.push_back(Record.Id);
            }
        auto Gate = std::make_shared<TaskGraph::TaskCompletionEvent>();
        auto Waits = ResourceLoader::instance()->GateResources(IDs, Gate);
        auto Task = InternalTask([this, Source, Destination, Root, Changes, ShaderEdits, Gate] {
            try
            {
                if (std::filesystem::exists(Root / Destination)) throw std::runtime_error("Move destination exists");
                std::filesystem::create_directories((Root / Destination).parent_path());
                CheckedPath(Root, Source); CheckedPath(Root, Destination);
                std::filesystem::rename(Root / Source, Root / Destination);
                try { for (const auto& Edit : ShaderEdits) AssetFile::AtomicWrite(Root / Edit.NewPath, {Edit.Updated.begin(), Edit.Updated.end()}); }
                catch (...)
                {
                    std::filesystem::rename(Root / Destination, Root / Source);
                    for (const auto& Edit : ShaderEdits) AssetFile::AtomicWrite(Root / Edit.OldPath, {Edit.Original.begin(), Edit.Original.end()});
                    throw;
                }
                for (const auto& [Id, Path] : Changes) ResourceLoader::instance()->RelocateResource(Id, Path);
                std::vector<std::string> Paths{Source, Destination};
                { std::lock_guard Lock(mMutex); ++mTicket;
                  for (const auto& [Id, NewPath] : Changes)
                  { auto It = mIDs.find(Id); if (It == mIDs.end()) continue; auto Old = mRecords.extract(It->second); if (Old.empty()) continue;
                    mTickets[Old.key()] = mTicket; Old.key() = NewPath; Old.mapped().Path = NewPath; mKnownPaths[Id] = NewPath; mTickets[NewPath] = mTicket; mRecords.insert(std::move(Old)); }
                  for (const auto& Edit : ShaderEdits) { mRecords[Edit.NewPath] = Read(Root, Edit.NewPath); mTickets[Edit.NewPath] = mTicket; }
                  Rebuild(); ++mRevision; }
                SaveCache(); ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed(); Refresh({});
                for (const auto& Edit : ShaderEdits) if (auto* Manager = ResourceLoader::instance()->GetManager(RT_Shader)) Manager->ReloadIfFileChanged((Root / Edit.NewPath).string());
            }
            catch (...) { ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed(); throw; }
        }, NamedThread::GameThread);
        Task->SetContinuationPolicy(TaskGraph::TaskContinuationPolicy::Always);
        for (auto& Wait : Waits) Task->Dependency(Wait);
        { std::lock_guard Lock(mMutex); if (mStopping) { ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed(); throw std::runtime_error("Registry is stopping"); }
          std::erase_if(mJobs, [](const auto& Job) { return Job->IsComplete(); }); mJobs.push_back(Task->GetCompletionEvent()); }
        TaskGraph::TaskGraph::instance()->Dispatch(Task); return Task->GetCompletionEvent();
    }
    void ResourceRegistry::ReserveImportPaths(const std::vector<std::string>& Paths)
    {
        std::lock_guard Lock(mMutex);
        if (mStopping) throw std::runtime_error("Registry is stopping");
        for (const auto& Path : Paths) if (mImportPaths.contains(Path)) throw std::runtime_error("Resource path is busy: " + Path);
        mImportPaths.insert(Paths.begin(), Paths.end());
    }
    void ResourceRegistry::ReleaseImportPaths(const std::vector<std::string>& Paths)
    { std::lock_guard Lock(mMutex); for (const auto& Path : Paths) mImportPaths.erase(Path); }
    void ResourceRegistry::PublishImported(const std::vector<ResourceDescriptor>& Records)
    {
        std::map<std::string, ResourceDescriptor> Changes;
        uint64_t Epoch, Ticket;
        { std::lock_guard Lock(mMutex);
          if (mStopping) throw std::runtime_error("Registry is stopping");
          Epoch = mEpoch; Ticket = ++mTicket;
          for (const auto& Record : Records) { Changes[Record.Path] = Record; mTickets[Record.Path] = Ticket; } }
        Publish(std::move(Changes), Epoch, Ticket, false);
    }
    TaskGraph::TaskCompletionEventPtr ResourceRegistry::Delete(const std::string& Path, bool AllowReferenced)
    {
        const auto Key = CanonicalResourcePath(Path); auto Root = RootPath(); CheckedPath(Root, Key); std::vector<ResourceId> IDs; std::vector<std::string> Paths;
        { std::lock_guard Lock(mMutex); for (const auto& Path : mImportPaths)
            if (Path == Key || Path.starts_with(Key + "/") || Key.starts_with(Path + "/")) throw std::runtime_error("Resource import is still in progress: " + Path); }
        for (const auto& Record : Query()) if (Record.Path == Key || Record.Path.starts_with(Key + "/"))
        {
            if (!AllowReferenced && !GetReferencers(Record.Id).empty()) throw std::runtime_error("Resource is still referenced: " + Record.Path);
            IDs.push_back(Record.Id); Paths.push_back(Record.Path);
        }
        auto Gate = std::make_shared<TaskGraph::TaskCompletionEvent>(); auto Waits = ResourceLoader::instance()->GateResources(IDs, Gate);
        auto Task = InternalTask([this, Root, Key, IDs, Paths, Gate] {
            try
            {
                for (const auto& Id : IDs) ResourceLoader::instance()->UnloadResource(Id);
                std::filesystem::remove_all(CheckedPath(Root, Key));
                { std::lock_guard Lock(mMutex); ++mTicket; for (const auto& Path : Paths) { mRecords.erase(Path); mTickets[Path] = mTicket; } Rebuild(); ++mRevision; }
                SaveCache(); ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed(); Refresh({});
            }
            catch (...) { ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed(); throw; }
        }, NamedThread::GameThread);
        Task->SetContinuationPolicy(TaskGraph::TaskContinuationPolicy::Always); for (auto& Wait : Waits) Task->Dependency(Wait);
        { std::lock_guard Lock(mMutex); if (mStopping) { ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed(); throw std::runtime_error("Registry is stopping"); }
          std::erase_if(mJobs, [](const auto& Job) { return Job->IsComplete(); }); mJobs.push_back(Task->GetCompletionEvent()); }
        TaskGraph::TaskGraph::instance()->Dispatch(Task); return Task->GetCompletionEvent();
    }
}
