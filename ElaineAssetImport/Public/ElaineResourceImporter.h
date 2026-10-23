#pragma once
#include "Resource/ElaineAsset.h"
#include <memory>

namespace Elaine
{
    struct ResourceImportSettings
    {
        virtual ~ResourceImportSettings() = default;
        virtual std::shared_ptr<const ResourceImportSettings> Clone() const = 0;
        virtual JsonCpp ToJson() const = 0;
    };

    struct ResourceImportRequest
    {
        std::string Importer;
        std::vector<std::filesystem::path> Sources;
        std::string Destination; // Relative to the service's asset root.
        std::shared_ptr<const ResourceImportSettings> Settings;
        bool Reimport = false;
    };

    struct ImportedAsset
    {
        std::string SourceKey;
        ResourceDescriptor Descriptor;
        AssetFile::Sections Sections;
        JsonCpp Data; // Business assets; null for binary containers.
    };

    // A conversion owns all of its CPU data. It never creates cached or GPU resources.
    struct ResourceImportOutput
    {
        std::vector<ImportedAsset> Assets; // Primary asset first.
        std::vector<std::string> Warnings;
    };

    struct ResourceImportContext
    {
        std::filesystem::path Root;
        ResourceImportRequest Request;
        std::map<std::string, ResourceDescriptor> Previous;
        ImportedAsset MakeAsset(const std::string& Key, const std::string& Type, const std::string& Path) const;
    };

    class ResourceImporter
    {
    public:
        virtual ~ResourceImporter() = default;
        virtual const char* GetName() const = 0;
        virtual std::vector<std::string> GetSupportedExtensions() const = 0;
        virtual JsonCpp GetSettingsDescription() const = 0;
        virtual std::shared_ptr<const ResourceImportSettings> ReadSettings(const JsonCpp&) const = 0;
        virtual ResourceImportOutput Build(const ResourceImportContext&) const = 0;
    };
}
