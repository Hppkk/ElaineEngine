#pragma once
#include "Resource/ElaineAsset.h"
#include "Resource/ElaineResourceLoad.h"
#include "TaskGraph/ElaineTask.h"
#include <mutex>

namespace Elaine
{
    struct ResourceQuery
    {
        std::string Directory, Search, Type, Tag, Value;
        bool Recursive = true;
    };

    class ElaineCoreExport ResourceRegistry
    {
    public:
        static ResourceRegistry& Get();
        TaskGraph::TaskCompletionEventPtr Start(const std::filesystem::path& Root);
        void Stop();
        TaskGraph::TaskCompletionEventPtr Refresh(const std::vector<std::string>& Paths);
        void WaitForStartup();
        std::vector<ResourceDescriptor> Query(const ResourceQuery& Query = {}) const;
        ResourceDescriptor Find(const ResourceId& Id) const;
        ResourceDescriptor FindByPath(const std::string& Path) const;
        ResourceReference Reference(const std::string& Path) const;
        std::string Resolve(const ResourceReference& Reference, const std::string& Type = {}) const;
        std::vector<ResourceDependency> GetDependencies(const ResourceId& Id) const;
        std::vector<ResourceDescriptor> GetReferencers(const ResourceId& Id) const;
        ResourceSubscription ObserveChanges(std::function<void()> Callback);
        TaskGraph::TaskCompletionEventPtr Move(const std::string& From, const std::string& To);
        TaskGraph::TaskCompletionEventPtr Delete(const std::string& Path, bool AllowReferenced = false);
        void PublishImported(const std::vector<ResourceDescriptor>& Records);
        void ReserveImportPaths(const std::vector<std::string>& Paths);
        void ReleaseImportPaths(const std::vector<std::string>& Paths);
        uint64_t Revision() const;
        std::filesystem::path RootPath() const;
    private:
        ResourceRegistry() = default;
        using Records = std::map<std::string, ResourceDescriptor>;
        static ResourceDescriptor Read(const std::filesystem::path& Root, const std::string& Path);
        void Publish(Records Records, uint64_t Epoch, uint64_t Ticket, bool Replace);
        void Rebuild();
        void SaveCache() const;
        void SynchronizeLocation(const ResourceId& Id);
        mutable std::mutex mMutex;
        std::filesystem::path mRoot;
        Records mRecords;
        std::map<ResourceId, std::string> mIDs, mKnownPaths;
        std::map<ResourceId, std::vector<std::string>> mReferencers;
        std::map<std::string, uint64_t> mTickets;
        std::vector<std::pair<std::shared_ptr<std::atomic_bool>, std::function<void()>>> mObservers;
        std::vector<TaskGraph::TaskCompletionEventPtr> mJobs;
        std::set<std::string> mImportPaths;
        TaskGraph::TaskCompletionEventPtr mReady;
        uint64_t mEpoch = 0, mTicket = 0, mRevision = 0;
        bool mStopping = true;
    };
}
