#include "ElainePrecompiledHeader.h"
#include "ElaineFileMonitor.h"
#include <chrono>
#include <algorithm>

namespace
{
	bool IsIgnoredFile(const std::filesystem::path& Path)
	{
		const std::string Name = Path.filename().string();
		if (Name.empty() || Name == "imgui.ini" || Name.back() == '~') return true;
		const std::string Ext = Path.extension().string();
		const std::string Full = Path.generic_string();
		return Ext == ".tmp" || Ext == ".swp" || Ext == ".bak" || Ext == ".pdb" ||
			Full.find("/Intermediate/") != std::string::npos || Full.find("/Binaries/") != std::string::npos ||
			Full.find("/build/") != std::string::npos;
	}
}

namespace Elaine
{
	FileMonitor::~FileMonitor()
	{
		Stop();
	}

	void FileMonitor::Start(const std::string& InRootPath, uint32_t InIntervalMs)
	{
		Stop();
		std::error_code Error;
		const auto Root = std::filesystem::weakly_canonical(std::filesystem::path(InRootPath), Error);
		mRootPath = Error ? std::filesystem::path(InRootPath).lexically_normal().string() : Root.string();
		mIntervalMs = std::max<uint32_t>(InIntervalMs, 16);
		{
			std::lock_guard<std::mutex> Lock(mMutex);
			mFileStamps.clear();
			mPendingChanges.clear();
		}
		if (!std::filesystem::is_directory(mRootPath, Error)) return;
		mRunning.store(true, std::memory_order_release);
		mThread = std::thread(&FileMonitor::WatchLoop, this);
	}

	void FileMonitor::Stop()
	{
		mRunning.store(false, std::memory_order_release);
		if (mThread.joinable()) mThread.join();
	}

	void FileMonitor::RegisterCallback(ChangeCallback InCallback)
	{
		if (!InCallback) return;
		std::lock_guard<std::mutex> Lock(mMutex);
		mCallbacks.push_back(std::move(InCallback));
	}

    void FileMonitor::DeferChanges(const std::vector<std::string>& Paths, bool Defer)
    {
        std::lock_guard Lock(mMutex);
        for (const auto& Path : Paths)
        {
            auto Key = std::filesystem::weakly_canonical(Path).string();
            if (Defer) mDeferredPaths.insert(Key); else mDeferredPaths.erase(Key);
        }
    }
	void FileMonitor::DispatchChanges()
	{
		std::vector<std::string> Changes;
		std::vector<ChangeCallback> Callbacks;
		{
			std::lock_guard<std::mutex> Lock(mMutex);
            for (auto It = mPendingChanges.begin(); It != mPendingChanges.end(); )
                if (mDeferredPaths.contains(*It)) ++It;
                else { Changes.push_back(std::move(*It)); It = mPendingChanges.erase(It); }
			Callbacks = mCallbacks;
		}
		for (const auto& Path : Changes)
			for (const auto& Callback : Callbacks)
				Callback(Path);
	}

	void FileMonitor::WatchLoop()
	{
        bool InitialSnapshot = true;
		while (mRunning.load(std::memory_order_acquire))
		{
			std::unordered_map<std::string, FileStamp> Current;
			std::vector<std::string> Changes;
			std::error_code Error;
			for (const auto& Entry : std::filesystem::recursive_directory_iterator(mRootPath, Error))
			{
				if (Error || !Entry.is_regular_file(Error)) continue;
				const auto PathObj = Entry.path();
				if (IsIgnoredFile(PathObj)) continue;
				const auto Stamp = Entry.last_write_time(Error).time_since_epoch().count();
				const auto Size = Entry.file_size(Error);
				if (Error) continue;
				const std::string Path = PathObj.lexically_normal().string();
				Current[Path] = { static_cast<uint64_t>(Stamp), Size };
				auto It = mFileStamps.find(Path);
				if (!InitialSnapshot && (It == mFileStamps.end() || It->second != Current[Path])) Changes.push_back(Path);
			}
			for (const auto& Old : mFileStamps)
				if (Current.find(Old.first) == Current.end()) Changes.push_back(Old.first);
			if (!Changes.empty())
			{
				std::lock_guard<std::mutex> Lock(mMutex);
				for (const auto& Path : Changes)
					if (std::find(mPendingChanges.begin(), mPendingChanges.end(), Path) == mPendingChanges.end())
						mPendingChanges.push_back(Path);
			}
			mFileStamps.swap(Current);
            InitialSnapshot = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(mIntervalMs));
		}
	}
}
