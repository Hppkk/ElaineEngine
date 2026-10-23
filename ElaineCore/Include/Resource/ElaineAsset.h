#pragma once
#include "ElaineCorePrerequirements.h"
#include "Elaine.h"
#include "nlohmann/json.hpp"
#include <array>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace Elaine
{
    struct ElaineCoreExport ResourceId
    {
        std::array<uint8_t, 16> Bytes{};
        static ResourceId New();
        static ResourceId Parse(const std::string& Text);
        std::string ToString() const;
        bool IsValid() const;
        bool operator==(const ResourceId&) const = default;
        bool operator<(const ResourceId& Other) const { return Bytes < Other.Bytes; }
    };

    struct ElaineCoreExport ResourceReference
    {
        ResourceId Id;
        std::string Path;
        static ResourceReference FromJson(const JsonCpp& Json);
        JsonCpp ToJson() const;
    };

    enum class ResourceDependencyKind { Hard, Soft };

    struct ElaineCoreExport ResourceDependency
    {
        ResourceReference Reference;
        ResourceDependencyKind Kind = ResourceDependencyKind::Hard;
        JsonCpp ToJson() const;
        static ResourceDependency FromJson(const JsonCpp& Json);
    };

    struct ElaineCoreExport ResourceDescriptor
    {
        ResourceId Id;
        std::string Type, Path, DisplayName;
        uint32_t DataVersion = 1;
        JsonCpp Tags = JsonCpp::object();
        std::vector<ResourceDependency> Dependencies;
        std::vector<std::string> SourceFiles;
        std::string Error;
        JsonCpp ExtraFields = JsonCpp::object();
        JsonCpp ToJson() const;
        static ResourceDescriptor FromJson(const JsonCpp& Json, const std::string& Path);
    };

    // Business assets are UTF-8 JSON; Texture/Mesh use explicit little-endian sections.
    // Binary descriptors can be read without bulk data. Neither format serializes native objects.
    class ElaineCoreExport AssetFile
    {
    public:
        using Sections = std::map<std::string, std::vector<uint8_t>>;
        static ResourceDescriptor ReadDescriptor(const std::filesystem::path& Path);
        static JsonCpp ReadMetadata(const std::filesystem::path& Path);
        static std::vector<uint8_t> ReadSection(const std::filesystem::path& Path, const std::string& Name);
        static JsonCpp ReadJson(const std::filesystem::path& Path, const std::string& ExpectedType = {});
        static void Write(const std::filesystem::path& Path, ResourceDescriptor Descriptor, const Sections& Data);
        static void WriteJson(const std::filesystem::path& Path, ResourceDescriptor Descriptor, JsonCpp Data);
        static std::vector<ResourceReference> CollectReferences(const JsonCpp& Data);
        static std::vector<ResourceDependency> CollectDependencies(const JsonCpp& Data, const std::string& Type);
        static void ValidateDependencies(const ResourceDescriptor& Descriptor, const JsonCpp& Data);
        static void AtomicWrite(const std::filesystem::path& Path, const std::vector<uint8_t>& Bytes);
    };

    std::string ElaineCoreExport CanonicalResourcePath(const std::string& Path);
    std::string ElaineCoreExport ResourceTypeName(uint8_t Type);
    uint8_t ElaineCoreExport ResourceTypeFromName(const std::string& Type);
}
