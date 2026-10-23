#include "ElainePrecompiledHeader.h"
#include "Resource/ElaineAsset.h"
#include <fstream>
#include <random>
#include <set>
#include <limits>

namespace Elaine
{
    namespace
    {
        constexpr uint64_t HeaderSize = 48;
        constexpr uint64_t MaxMetadata = 16 * 1024 * 1024;
        constexpr char Magic[8] = {'E','L','A','S','S','E','T',0};
        void Put(std::vector<uint8_t>& Bytes, uint64_t Value, unsigned Size)
        { for (unsigned i = 0; i < Size; ++i) Bytes.push_back(uint8_t(Value >> (i * 8))); }
        uint64_t Get(const uint8_t* Bytes, unsigned Size)
        { uint64_t Value = 0; for (unsigned i = 0; i < Size; ++i) Value |= uint64_t(Bytes[i]) << (i * 8); return Value; }
        void Read(std::ifstream& File, void* Data, size_t Size)
        { if (Size && !File.read(static_cast<char*>(Data), static_cast<std::streamsize>(Size))) throw std::runtime_error("Truncated asset"); }
        bool IsText(std::ifstream& File)
        {
            File.seekg(0);
            if (File.peek() == 0xef) { char Bom[3]; Read(File, Bom, 3); if (memcmp(Bom, "\xef\xbb\xbf", 3)) throw std::runtime_error("Invalid UTF-8 BOM"); }
            File >> std::ws;
            const bool Text = File.peek() == '{';
            File.clear(); File.seekg(0); return Text;
        }
        bool IsBusinessType(const std::string& Type)
        { return Type == "Material" || Type == "MaterialInstance" || Type == "Actor" || Type == "World" || Type == "ActorBundle"; }
        JsonCpp TextDocument(std::ifstream& File)
        {
            File.seekg(0, std::ios::end);
            if (File.tellg() > std::streamoff(MaxMetadata)) throw std::runtime_error("Text asset is too large");
            File.seekg(0); auto Json = JsonCpp::parse(File);
            if (!Json.is_object() || !Json.contains("resource") || !Json.contains("data") || !Json["data"].is_object())
                throw std::runtime_error("Text asset requires resource and data objects");
            const auto Descriptor = ResourceDescriptor::FromJson(Json["resource"], {});
            if (!IsBusinessType(Descriptor.Type)) throw std::runtime_error("Resource type requires a binary container: " + Descriptor.Type);
            AssetFile::ValidateDependencies(Descriptor, Json["data"]);
            return Json;
        }
        JsonCpp Metadata(std::ifstream& File)
        {
            if (IsText(File)) return TextDocument(File);
            File.seekg(0, std::ios::end);
            const auto End = File.tellg();
            if (End < std::streamoff(HeaderSize)) throw std::runtime_error("Missing or truncated asset header");
            const uint64_t Size = static_cast<uint64_t>(End);
            File.seekg(0);
            uint8_t Header[HeaderSize]; Read(File, Header, sizeof(Header));
            if (memcmp(Header, Magic, 8) || Get(Header + 8, 4) != 1 || Get(Header + 12, 4) != HeaderSize)
                throw std::runtime_error("Unsupported asset container");
            const uint64_t MetaSize = Get(Header + 16, 8), DataOffset = Get(Header + 24, 8), DataSize = Get(Header + 32, 8);
            if (MetaSize > MaxMetadata || MetaSize > Size - HeaderSize || DataOffset != HeaderSize + MetaSize ||
                DataSize != Size - DataOffset || Get(Header + 40, 8) != 0)
                throw std::runtime_error("Invalid asset section boundaries");
            std::string Text(static_cast<size_t>(MetaSize), '\0'); Read(File, Text.data(), Text.size());
            auto Json = JsonCpp::parse(Text);
            if (!Json.is_object() || !Json.contains("sections") || !Json["sections"].is_object()) throw std::runtime_error("Invalid asset metadata");
            uint64_t Expected = 0;
            std::vector<std::pair<uint64_t, uint64_t>> Ranges;
            for (const auto& Section : Json["sections"].items())
            {
                const auto& Node = Section.value();
                if (!Node.at("offset").is_number_unsigned() || !Node.at("size").is_number_unsigned()) throw std::runtime_error("Invalid asset section range");
                const auto Offset = Node.at("offset").get<uint64_t>(), Length = Node.at("size").get<uint64_t>();
                if (Offset > DataSize || Length > DataSize - Offset) throw std::runtime_error("Asset section exceeds file");
                if (Length) Ranges.emplace_back(Offset, Length);
            }
            std::sort(Ranges.begin(), Ranges.end());
            for (const auto& Range : Ranges)
            { if (Range.first != Expected) throw std::runtime_error("Overlapping or unaccounted asset data"); Expected += Range.second; }
            if (Expected != DataSize) throw std::runtime_error("Unaccounted asset data");
            Json["_dataOffset"] = DataOffset;
            return Json;
        }
    }

    ResourceId ResourceId::New()
    {
        ResourceId Id;
        std::random_device Random;
        for (auto& Byte : Id.Bytes) Byte = static_cast<uint8_t>(Random());
        Id.Bytes[6] = (Id.Bytes[6] & 15) | 64; Id.Bytes[8] = (Id.Bytes[8] & 63) | 128;
        return Id;
    }
    bool ResourceId::IsValid() const { return std::any_of(Bytes.begin(), Bytes.end(), [](auto Byte) { return Byte != 0; }); }
    ResourceId ResourceId::Parse(const std::string& Text)
    {
        if (Text.size() != 36) throw std::runtime_error("Invalid resource ID: " + Text);
        ResourceId Id; size_t Cursor = 0;
        auto Digit = [](char C) -> uint8_t {
            if (C >= '0' && C <= '9') return C - '0';
            if (C >= 'a' && C <= 'f') return C - 'a' + 10;
            if (C >= 'A' && C <= 'F') return C - 'A' + 10;
            throw std::runtime_error("Invalid resource ID digit");
        };
        for (size_t i = 0; i < Id.Bytes.size(); ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10) { if (Text[Cursor++] != '-') throw std::runtime_error("Invalid resource ID separator"); }
            Id.Bytes[i] = uint8_t((Digit(Text[Cursor]) << 4) | Digit(Text[Cursor + 1])); Cursor += 2;
        }
        if (!Id.IsValid()) throw std::runtime_error("Empty resource ID");
        return Id;
    }
    std::string ResourceId::ToString() const
    {
        static const char* Hex = "0123456789abcdef";
        std::string Text;
        for (size_t i = 0; i < Bytes.size(); ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10) Text += '-';
            Text += Hex[Bytes[i] >> 4]; Text += Hex[Bytes[i] & 15];
        }
        return Text;
    }
    ResourceReference ResourceReference::FromJson(const JsonCpp& Json)
    {
        if (!Json.is_object()) throw std::runtime_error("Resource reference requires an ID and readable path");
        return {ResourceId::Parse(Json.at("id").get<std::string>()), Json.value("path", "")};
    }
    JsonCpp ResourceReference::ToJson() const
    { if (!Id.IsValid()) throw std::runtime_error("Cannot serialize an empty resource reference"); return {{"id", Id.ToString()}, {"path", Path}}; }
    JsonCpp ResourceDependency::ToJson() const
    {
        auto Json = Reference.ToJson(); Json["kind"] = Kind == ResourceDependencyKind::Hard ? "hard" : "soft"; return Json;
    }
    ResourceDependency ResourceDependency::FromJson(const JsonCpp& Json)
    {
        const auto Kind = Json.at("kind").get<std::string>();
        if (Kind != "hard" && Kind != "soft") throw std::runtime_error("Unknown resource dependency kind: " + Kind);
        return {ResourceReference::FromJson(Json), Kind == "hard" ? ResourceDependencyKind::Hard : ResourceDependencyKind::Soft};
    }
    JsonCpp ResourceDescriptor::ToJson() const
    {
        JsonCpp Refs = JsonCpp::array(); for (const auto& Ref : Dependencies) Refs.push_back(Ref.ToJson());
        auto Json = ExtraFields;
        Json.update(JsonCpp{{"id", Id.ToString()}, {"type", Type}, {"displayName", DisplayName}, {"dataVersion", DataVersion},
            {"tags", Tags}, {"dependencies", Refs}, {"sourceFiles", SourceFiles}});
        return Json;
    }
    ResourceDescriptor ResourceDescriptor::FromJson(const JsonCpp& Json, const std::string& Path)
    {
        ResourceDescriptor Result;
        Result.ExtraFields = Json;
        for (const auto* Key : {"id", "type", "displayName", "dataVersion", "tags", "dependencies", "sourceFiles"}) Result.ExtraFields.erase(Key);
        Result.Id = ResourceId::Parse(Json.at("id").get<std::string>());
        Result.Type = Json.at("type").get<std::string>();
        if (Result.Type.empty()) throw std::runtime_error("Asset has no resource type");
        Result.Path = Path; Result.DisplayName = Json.value("displayName", std::filesystem::path(Path).stem().string());
        Result.DataVersion = Json.value("dataVersion", 1u); Result.Tags = Json.value("tags", JsonCpp::object());
        if (!Result.Tags.is_object()) throw std::runtime_error("Asset tags must be an object");
        const auto Nodes = Json.value("dependencies", JsonCpp::array());
        if (!Nodes.is_array()) throw std::runtime_error("Resource dependencies must be an array");
        std::set<ResourceId> Seen;
        for (const auto& Node : Nodes)
        {
            auto Dependency = ResourceDependency::FromJson(Node);
            if (!Seen.insert(Dependency.Reference.Id).second) throw std::runtime_error("Duplicate resource dependency: " + Dependency.Reference.Id.ToString());
            Result.Dependencies.push_back(std::move(Dependency));
        }
        Result.SourceFiles = Json.value("sourceFiles", std::vector<std::string>{});
        return Result;
    }
    std::string CanonicalResourcePath(const std::string& Text)
    {
        std::string Normalized = Text; std::replace(Normalized.begin(), Normalized.end(), '\\', '/');
        auto Path = std::filesystem::path(Normalized).lexically_normal();
        if (Path.empty() || Path.is_absolute()) throw std::runtime_error("Resource path must be relative: " + Text);
        for (const auto& Part : Path) if (Part == "..") throw std::runtime_error("Resource path escapes Contents: " + Text);
        return Path.generic_string();
    }
    std::string ResourceTypeName(uint8_t Type)
    { static const char* Names[] = {"Texture", "Material", "MaterialInstance", "Shader", "Mesh", "Actor"}; return Type < 6 ? Names[Type] : ""; }
    uint8_t ResourceTypeFromName(const std::string& Type)
    { for (uint8_t i = 0; i < 6; ++i) if (ResourceTypeName(i) == Type) return i; return 6; }
    JsonCpp AssetFile::ReadMetadata(const std::filesystem::path& Path)
    { std::ifstream File(Path, std::ios::binary); if (!File) throw std::runtime_error("Cannot open asset: " + Path.string()); return Metadata(File); }
    ResourceDescriptor AssetFile::ReadDescriptor(const std::filesystem::path& Path)
    {
        if (Path.extension() == ".shader")
        {
            if (std::filesystem::file_size(Path) > MaxMetadata) throw std::runtime_error("Shader descriptor is too large");
            std::ifstream File(Path, std::ios::binary); auto Json = JsonCpp::parse(File);
            auto Result = ResourceDescriptor::FromJson(Json.at("resource"), Path.generic_string());
            if (Result.Type != "Shader") throw std::runtime_error("Invalid shader descriptor type");
            Result.Tags["Stage"] = Json.at("stage");
            Result.SourceFiles = {CanonicalResourcePath(Json.at("source").get<std::string>())};
            return Result;
        }
        return ResourceDescriptor::FromJson(ReadMetadata(Path).at("resource"), Path.generic_string());
    }
    std::vector<uint8_t> AssetFile::ReadSection(const std::filesystem::path& Path, const std::string& Name)
    {
        std::ifstream File(Path, std::ios::binary); if (!File) throw std::runtime_error("Cannot open asset: " + Path.string());
        auto Json = Metadata(File);
        if (Json.contains("data"))
        {
            if (Name != "data") throw std::runtime_error("Text asset has no section: " + Name);
            const auto Text = Json["data"].dump(); return {Text.begin(), Text.end()};
        }
        const auto& Section = Json.at("sections").at(Name);
        const uint64_t Size = Section.at("size").get<uint64_t>();
        if (Size > std::numeric_limits<size_t>::max() || Size > uint64_t(std::numeric_limits<std::streamsize>::max())) throw std::runtime_error("Asset section too large");
        std::vector<uint8_t> Bytes(static_cast<size_t>(Size));
        File.seekg(static_cast<std::streamoff>(Json.at("_dataOffset").get<uint64_t>() + Section.at("offset").get<uint64_t>()));
        Read(File, Bytes.data(), Bytes.size()); return Bytes;
    }
    JsonCpp AssetFile::ReadJson(const std::filesystem::path& Path, const std::string& ExpectedType)
    {
        auto Document = ReadMetadata(Path);
        const auto Descriptor = ResourceDescriptor::FromJson(Document.at("resource"), Path.generic_string());
        if (!ExpectedType.empty() && Descriptor.Type != ExpectedType) throw std::runtime_error("Asset type mismatch: " + Path.string());
        if (!IsBusinessType(Descriptor.Type)) throw std::runtime_error("Resource has no JSON business data: " + Path.string());
        // Legacy business containers are read only by the migration tool.
        if (!Document.contains("data")) throw std::runtime_error("Business asset requires readable JSON: " + Path.string());
        return Document.at("data");
    }
    std::vector<ResourceReference> AssetFile::CollectReferences(const JsonCpp& Json)
    {
        std::map<ResourceId, ResourceReference> References;
        std::function<void(const JsonCpp&)> Visit = [&](const JsonCpp& Node) {
            if (Node.is_object())
            {
                if (Node.contains("id") && Node.contains("path") && Node["id"].is_string())
                { auto Ref = ResourceReference::FromJson(Node); References.emplace(Ref.Id, Ref); return; }
                for (const auto& Child : Node.items()) Visit(Child.value());
            }
            else if (Node.is_array()) for (const auto& Child : Node) Visit(Child);
        };
        Visit(Json); std::vector<ResourceReference> Result; for (const auto& Pair : References) Result.push_back(Pair.second); return Result;
    }
    std::vector<ResourceDependency> AssetFile::CollectDependencies(const JsonCpp& Data, const std::string& Type)
    {
        std::map<ResourceId, ResourceDependency> Dependencies;
        auto Add = [&](const JsonCpp& Node, ResourceDependencyKind Kind) {
            for (const auto& Ref : CollectReferences(Node))
            {
                auto [It, Inserted] = Dependencies.emplace(Ref.Id, ResourceDependency{Ref, Kind});
                if (!Inserted && Kind == ResourceDependencyKind::Hard) It->second.Kind = Kind;
            }
        };
        for (const auto& Entry : Data.items())
        {
            if (Type == "World" && Entry.key() == "Chunks")
            {
                for (const auto& Chunk : Entry.value()) for (const auto& Field : Chunk.items())
                    Add(Field.value(), Field.key() == "Actors" || Field.key() == "Bundle" ? ResourceDependencyKind::Soft : ResourceDependencyKind::Hard);
            }
            else Add(Entry.value(), Type == "ActorBundle" && Entry.key() == "Actors" ? ResourceDependencyKind::Soft : ResourceDependencyKind::Hard);
        }
        std::vector<ResourceDependency> Result;
        for (const auto& Pair : Dependencies) Result.push_back(Pair.second);
        return Result;
    }
    void AssetFile::ValidateDependencies(const ResourceDescriptor& Descriptor, const JsonCpp& Data)
    {
        const auto Expected = CollectDependencies(Data, Descriptor.Type);
        std::map<ResourceId, ResourceDependencyKind> Actual;
        for (const auto& Dependency : Descriptor.Dependencies)
            if (!Actual.emplace(Dependency.Reference.Id, Dependency.Kind).second) throw std::runtime_error("Duplicate resource dependency");
        for (const auto& Dependency : Expected)
        {
            auto It = Actual.find(Dependency.Reference.Id);
            if (It == Actual.end() || It->second != Dependency.Kind)
                throw std::runtime_error("Resource dependency summary missing or misclassified: " + Dependency.Reference.Id.ToString());
            Actual.erase(It);
        }
        if (!Actual.empty()) throw std::runtime_error("Resource dependency summary contains an unused reference: " + Actual.begin()->first.ToString());
    }
    void AssetFile::AtomicWrite(const std::filesystem::path& Path, const std::vector<uint8_t>& Bytes)
    {
        if (!Path.parent_path().empty()) std::filesystem::create_directories(Path.parent_path());
        auto Temporary = Path; Temporary += "." + ResourceId::New().ToString() + ".tmp";
        try
        {
            { std::ofstream File(Temporary, std::ios::binary | std::ios::trunc); if (!File) throw std::runtime_error("Cannot create asset temporary file");
              File.write(reinterpret_cast<const char*>(Bytes.data()), static_cast<std::streamsize>(Bytes.size())); File.flush();
              if (!File) throw std::runtime_error("Cannot write asset: " + Path.string()); }
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
            if (!MoveFileExW(Temporary.c_str(), Path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Cannot replace asset: " + Path.string());
#else
            std::filesystem::rename(Temporary, Path);
#endif
        }
        catch (...) { std::error_code Error; std::filesystem::remove(Temporary, Error); throw; }
    }
    void AssetFile::Write(const std::filesystem::path& Path, ResourceDescriptor Descriptor, const Sections& Data)
    {
        if (!Descriptor.Id.IsValid() || Descriptor.Type.empty()) throw std::runtime_error("Asset identity is missing");
        JsonCpp Json = {{"resource", Descriptor.ToJson()}, {"sections", JsonCpp::object()}};
        uint64_t Size = 0;
        for (const auto& [Name, Bytes] : Data)
        { Json["sections"][Name] = {{"offset", Size}, {"size", uint64_t(Bytes.size())}}; if (Bytes.size() > UINT64_MAX - Size) throw std::runtime_error("Asset too large"); Size += Bytes.size(); }
        const auto MetadataText = Json.dump();
        if (MetadataText.size() > MaxMetadata || Size > SIZE_MAX - HeaderSize - MetadataText.size()) throw std::runtime_error("Asset too large");
        std::vector<uint8_t> Bytes; Bytes.reserve(static_cast<size_t>(HeaderSize + MetadataText.size() + Size));
        Bytes.insert(Bytes.end(), std::begin(Magic), std::end(Magic)); Put(Bytes, 1, 4); Put(Bytes, HeaderSize, 4);
        Put(Bytes, MetadataText.size(), 8); Put(Bytes, HeaderSize + MetadataText.size(), 8); Put(Bytes, Size, 8); Put(Bytes, 0, 8);
        Bytes.insert(Bytes.end(), MetadataText.begin(), MetadataText.end());
        for (const auto& Pair : Data) Bytes.insert(Bytes.end(), Pair.second.begin(), Pair.second.end());
        AtomicWrite(Path, Bytes);
    }
    void AssetFile::WriteJson(const std::filesystem::path& Path, ResourceDescriptor Descriptor, JsonCpp Data)
    {
        if (!Descriptor.Id.IsValid() || !IsBusinessType(Descriptor.Type) || !Data.is_object()) throw std::runtime_error("Invalid JSON business asset");
        Descriptor.Dependencies = CollectDependencies(Data, Descriptor.Type);
        const auto Text = JsonCpp{{"resource", Descriptor.ToJson()}, {"data", std::move(Data)}}.dump(2) + "\n";
        if (Text.size() > MaxMetadata) throw std::runtime_error("Text asset is too large");
        AtomicWrite(Path, {Text.begin(), Text.end()});
    }
}
