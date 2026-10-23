#include "ElainePrecompiledHeader.h"
#include "Resource/ElaineAsset.h"
#include "ElaineResourceImportService.h"
#include <fstream>
#include <iostream>
#include <set>

using namespace Elaine;
namespace fs = std::filesystem;
namespace
{
    std::vector<uint8_t> ReadBytes(const fs::path& Path)
    {
        std::ifstream File(Path, std::ios::binary | std::ios::ate);
        if (!File) throw std::runtime_error("Cannot read " + Path.string());
        const auto Size = File.tellg(); if (Size < 0) throw std::runtime_error("Cannot size file");
        std::vector<uint8_t> Bytes(static_cast<size_t>(Size)); File.seekg(0);
        if (!Bytes.empty() && !File.read(reinterpret_cast<char*>(Bytes.data()), Size)) throw std::runtime_error("Cannot read file"); return Bytes;
    }
    JsonCpp ReadJson(const fs::path& Path) { auto Bytes = ReadBytes(Path); while (!Bytes.empty() && !Bytes.back()) Bytes.pop_back(); return JsonCpp::parse(Bytes.begin(), Bytes.end()); }
    void JsonFile(const fs::path& Path, const JsonCpp& Json) { const auto Text = Json.dump(2); AssetFile::AtomicWrite(Path, {Text.begin(), Text.end()}); }
    std::string OldType(const fs::path& Path)
    {
        const auto Extension = Path.extension().string();
        if (Extension == ".tex" || Extension == ".texture") return "Texture";
        if (Extension == ".mesh" || Extension == ".emesh") return "Mesh";
        if (Extension == ".material") return "Material";
        if (Extension == ".mi") return "MaterialInstance";
        if (Extension == ".actor") return "Actor";
        if (Extension == ".world") return "World";
        if (Extension == ".vs" || Extension == ".vert" || Extension == ".ps" || Extension == ".frag" || Extension == ".gs" || Extension == ".cs") return "Shader";
        return {};
    }
    void Validate(const fs::path& Root)
    {
        std::map<ResourceId, ResourceDescriptor> Assets;
        for (const auto& Entry : fs::recursive_directory_iterator(Root))
        {
            if (!Entry.is_regular_file() || (Entry.path().extension() != ".asset" && Entry.path().extension() != ".shader")) continue;
            auto Descriptor = AssetFile::ReadDescriptor(Entry.path()); Descriptor.Path = fs::relative(Entry.path(), Root).generic_string();
            if (!Assets.emplace(Descriptor.Id, Descriptor).second) throw std::runtime_error("Duplicate resource ID: " + Descriptor.Id.ToString());
            if (Descriptor.Type == "Texture")
            {
                const auto LayoutBytes = AssetFile::ReadSection(Entry.path(), "texture"); const auto Layout = JsonCpp::parse(LayoutBytes.begin(), LayoutBytes.end());
                const auto Pixels = AssetFile::ReadSection(Entry.path(), "data");
                const uint64_t Expected = Layout.at("width").get<uint64_t>() * Layout.at("height").get<uint64_t>() * Layout.at("layers").get<uint64_t>() * Layout.at("faces").get<uint64_t>() * (Layout.at("format") == "RGBA32F" ? 16 : 4);
                if (Expected != Pixels.size()) throw std::runtime_error("Texture size mismatch");
            }
            else if (Descriptor.Type == "Shader")
            { auto Json = ReadJson(Entry.path()); if (!fs::is_regular_file(Root / CanonicalResourcePath(Json.at("source")))) throw std::runtime_error("Missing shader source"); }
            else if (Descriptor.Type == "Mesh")
            {
                auto Bytes = AssetFile::ReadSection(Entry.path(), "mesh"); auto Mesh = JsonCpp::parse(Bytes.begin(), Bytes.end());
                AssetFile::ValidateDependencies(Descriptor, Mesh);
                const auto Expected = BuildMeshAssetMetadata(AssetFile::ReadSection(Entry.path(), "data"));
                if (Expected.at("bounds") != Mesh.at("bounds")) throw std::runtime_error("Mesh bounds do not match geometry");
                if (Expected.at("subMeshes") != Mesh.at("subMeshes")) throw std::runtime_error("SubMesh bounds do not match geometry");
            }
            else
            {
                auto Json = AssetFile::ReadJson(Entry.path());
                AssetFile::ValidateDependencies(Descriptor, Json);
            }
        }
        for (const auto& [Id, Asset] : Assets) for (const auto& Ref : Asset.Dependencies)
            if (!Assets.contains(Ref.Reference.Id)) throw std::runtime_error("Unresolved reference in " + Asset.Path + ": " + Ref.Reference.Id.ToString());
        std::cout << "Validated " << Assets.size() << " assets and all direct references\n";
    }
    // The previous business container reader is confined to this conversion command.
    void MakeReadable(const fs::path& RootArg)
    {
        const auto Root = fs::canonical(RootArg);
        const auto Backup = Root.parent_path() / "Intermediate/ReadableAssetsBackup" / ResourceId::New().ToString();
        JsonCpp Manifest = {{"assets", JsonCpp::array()}};
        struct Conversion { fs::path Path; std::vector<uint8_t> Original; ResourceDescriptor Descriptor; JsonCpp Data; };
        std::vector<Conversion> Changes;
        for (const auto& Entry : fs::recursive_directory_iterator(Root))
        {
            if (!Entry.is_regular_file() || Entry.path().extension() != ".asset") continue;
            auto Metadata = AssetFile::ReadMetadata(Entry.path());
            if (Metadata.contains("data")) continue;
            auto Resource = Metadata.at("resource"); const auto Type = Resource.at("type").get<std::string>();
            if (Type != "Material" && Type != "MaterialInstance" && Type != "Actor" && Type != "World" && Type != "ActorBundle") continue;
            auto Bytes = AssetFile::ReadSection(Entry.path(), "data"); auto Data = JsonCpp::parse(Bytes.begin(), Bytes.end());
            const auto Relative = fs::relative(Entry.path(), Root);
            auto Dependencies = JsonCpp::array();
            for (const auto& Dependency : AssetFile::CollectDependencies(Data, Type)) Dependencies.push_back(Dependency.ToJson());
            Resource["dependencies"] = std::move(Dependencies);
            auto Descriptor = ResourceDescriptor::FromJson(Resource, Relative.generic_string());
            Changes.push_back({Relative, ReadBytes(Entry.path()), std::move(Descriptor), std::move(Data)});
        }
        if (Changes.empty()) { Validate(Root); return; }
        // Stage and verify every conversion before touching Contents.
        for (const auto& Change : Changes)
        {
            AssetFile::AtomicWrite(Backup / "assets" / Change.Path, Change.Original);
            AssetFile::WriteJson(Backup / "staging" / Change.Path, Change.Descriptor, Change.Data);
            if (AssetFile::ReadJson(Backup / "staging" / Change.Path) != Change.Data ||
                AssetFile::ReadDescriptor(Backup / "staging" / Change.Path).Id != Change.Descriptor.Id)
                throw std::runtime_error("Readable asset conversion verification failed");
            Manifest["assets"].push_back({{"path", Change.Path.generic_string()}, {"id", Change.Descriptor.Id.ToString()}});
        }
        JsonFile(Backup / "manifest.json", Manifest);
        try
        {
            for (const auto& Change : Changes)
                AssetFile::AtomicWrite(Root / Change.Path, ReadBytes(Backup / "staging" / Change.Path));
            Validate(Root);
        }
        catch (...)
        {
            for (const auto& Change : Changes) AssetFile::AtomicWrite(Root / Change.Path, Change.Original);
            throw;
        }
        Manifest["completed"] = true; JsonFile(Backup / "manifest.json", Manifest);
        std::cout << "Converted " << Changes.size() << " readable business assets; backup: " << Backup.generic_string() << '\n';
    }
    void Migrate(const fs::path& RootArg, bool Apply)
    {
        const auto Root = fs::canonical(RootArg);
        std::map<std::string, ResourceDescriptor> Plan; std::set<std::string> Destinations;
        JsonCpp Manifest = {{"version", 1}, {"root", Root.generic_string()}, {"assets", JsonCpp::array()}, {"removedSources", JsonCpp::array()}};
        for (const auto& Entry : fs::recursive_directory_iterator(Root))
        {
            if (!Entry.is_regular_file()) continue; const auto Type = OldType(Entry.path()); if (Type.empty()) continue;
            const auto Source = fs::relative(Entry.path(), Root).generic_string(); auto Target = fs::path(Source);
            if (Type == "Shader") Target += ".shader"; else Target.replace_extension(".asset");
            ResourceDescriptor Descriptor; Descriptor.Id = ResourceId::New(); Descriptor.Type = Type;
            Descriptor.Path = Target.generic_string(); Descriptor.DisplayName = Entry.path().stem().string();
            if (fs::exists(Root / Target) || !Destinations.insert(Descriptor.Path).second) throw std::runtime_error("Migration destination conflict: " + Descriptor.Path);
            Plan[Source] = Descriptor;
            Manifest["assets"].push_back({{"source", Source}, {"destination", Descriptor.Path}, {"id", Descriptor.Id.ToString()}, {"type", Type}});
            std::cout << Source << " -> " << Descriptor.Path << '\n';
        }
        if (!Apply || Plan.empty()) { if (Plan.empty()) Validate(Root); return; }
        const auto Backup = Root.parent_path() / "Intermediate/AssetMigrationBackup" / ResourceId::New().ToString();
        const auto Stage = Backup / "staging"; fs::create_directories(Stage);
        auto RefFor = [&](const std::string& Value) -> JsonCpp {
            auto It = Plan.find(Value);
            if (It == Plan.end()) It = Plan.find("shader/vulkan/" + Value);
            if (It == Plan.end()) throw std::runtime_error("Unresolved old resource reference: " + Value);
            return ResourceReference{It->second.Id, It->second.Path}.ToJson();
        };
        std::function<void(JsonCpp&)> Convert = [&](JsonCpp& Node) {
            if (Node.is_string())
            {
                const auto Value = Node.get<std::string>();
                if (Plan.contains(Value) || Plan.contains("shader/vulkan/" + Value)) Node = RefFor(Value);
                else if (!Value.empty() && !OldType(Value).empty()) throw std::runtime_error("Unresolved old reference: " + Value);
            }
            else if (Node.is_object()) for (auto& Entry : Node.items()) Convert(Entry.value());
            else if (Node.is_array()) for (auto& Entry : Node) Convert(Entry);
        };
        std::set<std::string> Remove;
        for (auto& [Source, Descriptor] : Plan)
        {
            const auto Target = Stage / Descriptor.Path;
            if (Descriptor.Type == "Shader")
            {
                const auto Ext = fs::path(Source).extension().string();
                const std::string StageName = Ext == ".vs" || Ext == ".vert" ? "vertex" : Ext == ".ps" || Ext == ".frag" ? "fragment" : Ext == ".gs" ? "geometry" : "compute";
                JsonFile(Target, {{"resource", Descriptor.ToJson()}, {"stage", StageName}, {"source", Source}});
            }
            else if (Descriptor.Type == "Texture")
            {
                const auto Json = ReadJson(Root / Source); TextureImportSettings Settings;
                Settings.Dimension = Json.at("dimension"); Settings.Usage = Json.value("usage", "color"); Settings.SRGB = Json.value("srgb", false);
                if (Json.value("nummips", 1) != 1) throw std::runtime_error("Old texture requires unavailable mip data");
                std::vector<fs::path> Sources; const auto& Original = Json.at("source");
                if (Settings.Dimension == "2d") Sources.push_back(Root / CanonicalResourcePath(Original.at("path")));
                else if (Settings.Dimension == "cube") for (const char* Face : {"px","nx","py","ny","pz","nz"}) Sources.push_back(Root / CanonicalResourcePath(Original.at(Face)));
                else if (Settings.Dimension == "2darray") for (const auto& Path : Original.at("paths")) Sources.push_back(Root / CanonicalResourcePath(Path));
                else throw std::runtime_error("Unsupported old texture layout");
                AssetFile::Write(Target, Descriptor, {});
                ResourceImportService Importer(Target.parent_path());
                auto Result = Importer.ImportTextureBlocking(Sources, Target.filename().string(), Settings);
                if (!Result.Succeeded()) throw std::runtime_error(Result.Error);
                for (size_t i=0;i<Sources.size();++i)
                {
                    if (ReadBytes(Sources[i]) != AssetFile::ReadSection(Target, "source"+std::to_string(i))) throw std::runtime_error("Embedded source verification failed");
                    Remove.insert(fs::relative(Sources[i], Root).generic_string());
                }
            }
            else if (Descriptor.Type == "Mesh")
            {
                auto Bytes = ReadBytes(Root / Source);
                if (Bytes.size() < 28) throw std::runtime_error("Truncated old mesh");
                auto U32 = [&](size_t Offset) { uint32_t Value = 0; for(size_t i=0;i<4;++i) Value |= uint32_t(Bytes.at(Offset+i)) << (i*8); return Value; };
                Descriptor.Tags = {{"VertexBuffers", U32(8)}, {"SubMeshes", U32(12)}, {"IndexCount", U32(16)}, {"IndexStride", U32(20)}, {"MaterialSlots", U32(24)}};
                AssetFile::Write(Target, Descriptor, {{"data", Bytes}});
                if (Bytes != AssetFile::ReadSection(Target,"data")) throw std::runtime_error("Mesh payload changed during migration");
            }
            else
            {
                auto Json = ReadJson(Root / Source); Convert(Json);
                if (Descriptor.Type == "Material") Descriptor.Tags = {{"Parameters", Json.value("parameters", JsonCpp::object()).size()}, {"Passes", Json.value("shaderpass",JsonCpp::array()).size()}};
                if (Descriptor.Type == "MaterialInstance") Descriptor.Tags = {{"Overrides",Json.value("parameters",JsonCpp::object()).size()}};
                if (Descriptor.Type == "Actor") Descriptor.Tags = {{"Components",Json.value("Components",JsonCpp::array()).size()}, {"Children",Json.value("Children",JsonCpp::array()).size()}};
                if (Descriptor.Type == "World") Descriptor.Tags = {{"Chunks",Json.value("Chunks",JsonCpp::array()).size()}};
                AssetFile::WriteJson(Target, Descriptor, Json);
                if (AssetFile::ReadJson(Target) != Json) throw std::runtime_error("JSON round-trip verification failed");
            }
            if (Descriptor.Type != "Shader") Remove.insert(Source);
        }
        // Validate the staged reference graph. Shader sources remain in Contents.
        std::set<ResourceId> IDs; for(const auto& Pair:Plan) IDs.insert(Pair.second.Id);
        for(const auto& Pair:Plan) for(const auto& Ref:AssetFile::ReadDescriptor(Stage/Pair.second.Path).Dependencies)
            if(!IDs.contains(Ref.Reference.Id)) throw std::runtime_error("Staged asset has an unresolved reference");
        for(const auto& Path:Remove)
        {
            const auto Original=fs::weakly_canonical(Root/Path);
            if(Original.lexically_relative(Root).empty() || Original.lexically_relative(Root).generic_string().starts_with("..")) throw std::runtime_error("Migration source escapes Contents");
            fs::create_directories((Backup/"originals"/Path).parent_path()); fs::copy_file(Original,Backup/"originals"/Path);
        }
        for(const auto& Path:Remove) if(!Plan.contains(Path)) Manifest["removedSources"].push_back(Path);
        JsonFile(Backup/"manifest.json",Manifest);
        std::vector<fs::path> Created;
        try
        {
            for(const auto& Pair:Plan)
            {
                const auto Destination=Root/Pair.second.Path;
                if(fs::exists(Destination)) throw std::runtime_error("Migration destination appeared during conversion");
                AssetFile::AtomicWrite(Destination,ReadBytes(Stage/Pair.second.Path)); Created.push_back(Destination);
            }
            Validate(Root);
            for(const auto& Path:Remove) fs::remove(Root/Path);
        }
        catch(...)
        {
            for(const auto& Path:Remove) if(!fs::exists(Root/Path)) fs::copy_file(Backup/"originals"/Path,Root/Path);
            for(const auto& Path:Created) fs::remove(Path); throw;
        }
        Manifest["completed"]=true; JsonFile(Backup/"manifest.json",Manifest);
        std::cout<<"Migration complete. Backup and manifest: "<<Backup.generic_string()<<'\n';
    }
}
int main(int Argc,char** Argv)
{
    try
    {
        if(Argc<3) { std::cout<<"ElaineAssetTool migrate <Contents> [--apply] | readable <Contents> | validate <Contents> | inspect <asset> | import <Contents> <source> <destination.asset> [--scale N] [--srgb] [--dimension 2d|cube|2darray] [--source image ...] [--usage color|normal|data|hdr] | reimport <Contents> <relative.asset> | upgrade-mesh <Contents>\n";return 2; }
        const std::string Command=Argv[1];
        if(Command=="migrate") Migrate(Argv[2],Argc>3&&std::string(Argv[3])=="--apply");
        else if(Command=="readable") MakeReadable(Argv[2]);
        else if(Command=="validate") Validate(Argv[2]);
        else if(Command=="inspect") std::cout<<AssetFile::ReadDescriptor(Argv[2]).ToJson().dump(2)<<'\n';
        else if(Command=="upgrade-mesh")
        { size_t Count=0; for (const auto& Entry:fs::recursive_directory_iterator(Argv[2]))
            if (Entry.is_regular_file() && Entry.path().extension()==".asset" && UpgradeMeshAsset(Entry.path())) ++Count;
          std::cout<<"Upgraded "<<Count<<" Mesh assets\n"; }
        else if(Command=="import" || Command=="reimport")
        {
            if (Argc < (Command=="import"?5:4)) throw std::runtime_error("Missing import arguments");
            ResourceImportService Service(Argv[2]); ResourceImportResult Result;
            if (Command=="reimport") Result=Service.ReimportBlocking(Argv[3]);
            else
            {
                std::string Extension=fs::path(Argv[3]).extension().string();
                std::transform(Extension.begin(),Extension.end(),Extension.begin(),[](unsigned char C){return static_cast<char>(std::tolower(C));});
                const auto Extensions=Service.GetSupportedExtensions("Texture");
                if (std::find(Extensions.begin(),Extensions.end(),Extension)!=Extensions.end())
                {
                    TextureImportSettings Settings; std::vector<fs::path> Sources{Argv[3]};
                    for(int I=5;I<Argc;++I)
                    {
                        const std::string Option=Argv[I];
                        if(Option=="--srgb") Settings.SRGB=true;
                        else if(Option=="--source" && I+1<Argc) Sources.emplace_back(Argv[++I]);
                        else if(Option=="--dimension" && I+1<Argc) Settings.Dimension=Argv[++I];
                        else if(Option=="--usage" && I+1<Argc) Settings.Usage=Argv[++I];
                        else throw std::runtime_error("Unknown or incomplete texture import option: "+Option);
                    }
                    Result=Service.ImportTextureBlocking(Sources,Argv[4],Settings);
                }
                else
                { MeshImportSettings Settings; for(int I=5;I<Argc;++I) if(std::string(Argv[I])=="--scale" && I+1<Argc) Settings.Scale=std::stof(Argv[++I]); else throw std::runtime_error("Unknown mesh import option");
                  Result=Service.ImportBlocking({"Mesh",{Argv[3]},Argv[4],Settings.Clone(),false}); }
            }
            if(!Result.Succeeded()) throw std::runtime_error(Result.Error);
            for(const auto& Asset:Result.Assets) std::cout<<Asset.Type<<" "<<Asset.Path<<" "<<Asset.Id.ToString()<<'\n';
            for(const auto& Asset:Result.UnusedAssets) std::cout<<"Unused (retained): "<<Asset.Path<<'\n';
            for(const auto& Warning:Result.Warnings) std::cout<<"Warning: "<<Warning<<'\n';
        }
        else throw std::runtime_error("Unknown asset tool command");
        return 0;
    }
    catch(const std::exception& Error) {std::cerr<<Error.what()<<'\n';return 1;}
}
