#pragma once
#include "ElaineTextureImporter.h"
#include "ElaineMeshImporter.h"
#include "Resource/ElaineResourceRegistry.h"

namespace Elaine
{
    struct ResourceImportResult
    {
        ResourceDescriptor Primary;
        std::vector<ResourceDescriptor> Assets;
        std::vector<ResourceDescriptor> UnusedAssets;
        std::vector<std::string> Warnings;
        std::string Error;
        bool Succeeded() const { return Error.empty() && Primary.Id.IsValid(); }
    };
    using ResourceImportCompletion = std::function<void(const ResourceImportResult&)>;

    class ResourceImportService
    {
    public:
        explicit ResourceImportService(std::filesystem::path Root, ResourceRegistry* Registry = nullptr);
        ~ResourceImportService();
        static ResourceImportService& Get();
        void RegisterImporter(std::unique_ptr<ResourceImporter> Importer);
        std::vector<std::string> GetSupportedExtensions(const std::string& Importer) const;
        JsonCpp GetSettingsDescription(const std::string& Importer) const;
        TaskGraph::TaskCompletionEventPtr Import(ResourceImportRequest Request, ResourceImportCompletion Completion = {});
        TaskGraph::TaskCompletionEventPtr ImportTexture(const std::vector<std::filesystem::path>& Sources, const std::string& Destination,
            const TextureImportSettings& Settings = {}, ResourceImportCompletion Completion = {});
        TaskGraph::TaskCompletionEventPtr ImportMesh(const std::filesystem::path& Source, const std::string& Destination,
            const MeshImportSettings& Settings = {}, ResourceImportCompletion Completion = {});
        TaskGraph::TaskCompletionEventPtr Reimport(const std::string& Path, ResourceImportCompletion Completion = {});
        ResourceImportResult ImportBlocking(ResourceImportRequest Request);
        ResourceImportResult ImportTextureBlocking(const std::vector<std::filesystem::path>& Sources, const std::string& Destination,
            const TextureImportSettings& Settings = {});
        ResourceImportResult ReimportBlocking(const std::string& Path, std::shared_ptr<const ResourceImportSettings> Settings = {});
        void Shutdown();
    private:
        struct Operation;
        std::shared_ptr<Operation> Prepare(ResourceImportRequest Request);
        void Convert(Operation&);
        void Reserve(Operation&);
        void ScheduleCommit(const std::shared_ptr<Operation>&, const TaskGraph::TaskCompletionEventPtr&, size_t Index = 0);
        void Commit(Operation&);
        void Release(Operation&);
        void Publish(Operation&);
        const ResourceImporter& Find(const std::string&) const;
        std::filesystem::path mRoot;
        ResourceRegistry* mRegistry;
        std::map<std::string, std::unique_ptr<ResourceImporter>> mImporters;
        std::mutex mMutex;
        std::set<std::string> mReserved;
        std::set<std::string> mRequests;
        std::shared_ptr<std::atomic_bool> mCallbacksActive = std::make_shared<std::atomic_bool>(true);
        std::vector<TaskGraph::TaskCompletionEventPtr> mJobs;
        bool mStopping = false;
    };
}
