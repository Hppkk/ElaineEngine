#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"
#include <filesystem>
#include <functional>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <set>

namespace Elaine
{
	class ElaineCoreExport FileMonitor : public Singleton<FileMonitor>
	{
	public:
		FileMonitor() = default;
		~FileMonitor();
		void Start(const std::string& InRootPath, uint32_t InIntervalMs = 250);
		void Stop();
		void DispatchChanges();
        void DeferChanges(const std::vector<std::string>& Paths, bool Defer);
		using ChangeCallback = std::function<void(const std::string&)>;
		void RegisterCallback(ChangeCallback InCallback);
		bool IsRunning() const { return mRunning.load(); }
	private:
		void WatchLoop();
		std::string mRootPath;
		uint32_t mIntervalMs = 250;
		std::atomic<bool> mRunning{false};
		std::thread mThread;
		std::mutex mMutex;
		struct FileStamp
		{
			uint64_t WriteTime = 0;
			uintmax_t Size = 0;
			bool operator==(const FileStamp& Other) const { return WriteTime == Other.WriteTime && Size == Other.Size; }
		};
		std::unordered_map<std::string, FileStamp> mFileStamps;
		std::vector<std::string> mPendingChanges;
        std::set<std::string> mDeferredPaths;
		std::vector<ChangeCallback> mCallbacks;
	};
}
