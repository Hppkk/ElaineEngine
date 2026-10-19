#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineMeshRenderProxy.h"
#include "ElaineRenderContext.h"

namespace Elaine
{
	StaticMeshRenderProxy::StaticMeshRenderProxy()
	{
		mType = EProxyType::StaticMesh;
	}

	StaticMeshRenderProxy::~StaticMeshRenderProxy()
	{

	}

	void StaticMeshRenderProxy::UpdateMaterials(const std::vector<MaterialParamSnapshot>& InSnapshots)
	{
		// 调整代理数组大小
		mMaterialProxies.resize(InSnapshots.size());

		// 逐个从快照更新
		for (size_t i = 0; i < InSnapshots.size(); ++i)
		{
			mMaterialProxies[i].UpdateFromSnapshot(InSnapshots[i]);
			mMaterialProxies[i].BindToResourceBinding(mResourceBinding);
		}
	}

	void StaticMeshRenderProxy::UpdateMaterials(const std::vector<MaterialParamSnapshot>& InSnapshots, RenderContext& InContext)
	{
		mMaterialProxies.resize(InSnapshots.size());
		for (size_t i = 0; i < InSnapshots.size(); ++i)
		{
			mMaterialProxies[i].UpdateFromSnapshot(InSnapshots[i], InContext);
			mMaterialProxies[i].BindToResourceBinding(mResourceBinding);
		}
	}

	RenderMaterialProxy* StaticMeshRenderProxy::GetMaterialProxy(uint32 InIndex)
	{
		if (InIndex >= mMaterialProxies.size())
			return nullptr;
		return &mMaterialProxies[InIndex];
	}

	const RenderMaterialProxy* StaticMeshRenderProxy::GetMaterialProxy(uint32 InIndex) const
	{
		if (InIndex >= mMaterialProxies.size())
			return nullptr;
		return &mMaterialProxies[InIndex];
	}

	void StaticMeshRenderProxy::UpdateRenderQueue(RenderQueueSet* InRenderQueue)
	{
		if (!IsBindingsInitialized())
			return;

	}

}
