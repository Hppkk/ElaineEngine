#pragma once
#include "ElaineSingleton.h"
#include "ElaineResourceBase.h"

namespace Elaine
{
	struct ResourceMemoryStats
	{
		uint64_t CurrentBytes = 0;
		uint64_t PeakBytes = 0;
		uint64_t LoadedResources = 0;
		uint64_t TotalAllocatedBytes = 0;
	};
	enum ResourceType : uint8_t
	{
		RT_Texture,
		RT_Material,
		RT_MaterialInstance,
		RT_Shader,
		RT_Mesh,
		RT_Actor,
		RT_Count
	};

	class ResourceBase;
	class ElaineCoreExport ResourceManager
	{
	public:
		ResourceManager(ResourceType InType);
		virtual ~ResourceManager();

		virtual ResourceBasePtr	GetResource(const std::string& InPath, bool InAsync = true);
		ResourceBasePtr CreateEmptyResource(const std::string& InPath);
		ResourceMemoryStats GetMemoryStats() const;
		void OnResourceMemoryChanged(int64_t Delta, size_t AllocatedBytes = 0);
		void OnResourceLoaded(bool Loaded);

		template<typename Ty>
		ResourcePtr<Ty> GetResource(const std::string& InPath, bool InAsync = true)
		{
			return static_pointer_cast<Ty, ResourceBase>(GetResource(InPath, InAsync));
		}
		template<typename Ty>
		ResourcePtr<Ty> CreateEmptyResource(const std::string& InPath)
		{
			return static_pointer_cast<Ty, ResourceBase>(CreateEmptyResource(InPath));
		}
	protected:
		virtual	ResourceBasePtr CreateResourceImpl(const std::string& InPath) = 0;
	protected:
		ResourceType mResourceType;
		std::map<std::string, ResourceBasePtr> mResources;
		std::mutex mMtx;
		std::atomic<uint64_t> mCurrentBytes{0};
		std::atomic<uint64_t> mPeakBytes{0};
		std::atomic<uint64_t> mLoadedResources{0};
		std::atomic<uint64_t> mTotalAllocatedBytes{0};
	};
}
