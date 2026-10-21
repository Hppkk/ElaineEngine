#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineMeshRenderProxy.h"
#include "ElaineRenderContext.h"
#include "ElaineRenderQueue.h"

namespace Elaine
{
	StaticMeshRenderProxy::StaticMeshRenderProxy()
	{
		mType = EProxyType::StaticMesh;
	}

	StaticMeshRenderProxy::~StaticMeshRenderProxy()
    {
        CancelResourceNotifications();
    }

    void StaticMeshRenderProxy::SetMesh(const MeshPtr& InMesh)
    {
        mMeshPath = InMesh ? InMesh->GetPath() : std::string();
        mMesh = InMesh;
        if (InMesh) TrackResource(InMesh);
        BeginInitialization();
    }
    void StaticMeshRenderProxy::InitializeResourceBinding()
    {
        mSubMeshCount = static_cast<uint32>(mMeshData.SubMeshes.size());
    }
    void StaticMeshRenderProxy::ApplyResourceUpdate(const RenderResourceUpdate& Update)
    {
        if (Update.IsMesh && Update.Resource.get() == mMesh.get())
        {
            mMeshData = Update.Mesh;
            mSubMeshCount = static_cast<uint32>(mMeshData.SubMeshes.size());
        }
        for (auto& Material : mMaterialProxies) Material.ApplyResourceUpdate(Update);
    }

	void StaticMeshRenderProxy::UpdateMaterials(const std::vector<MaterialParamSnapshot>& InSnapshots)
	{
		// 调整代理数组大小
		mMaterialProxies.resize(InSnapshots.size());
        for (const auto& Snapshot : InSnapshots)
        {
            if (Snapshot.Source) TrackResource(Snapshot.Source);
            for (const auto& Pair : Snapshot.OverridedTextures) if (Pair.second) TrackResource(Pair.second);
        }

		// 逐个从快照更新
		for (size_t i = 0; i < InSnapshots.size(); ++i)
		{
			mMaterialProxies[i].UpdateFromSnapshot(InSnapshots[i]);
		}
	}

	void StaticMeshRenderProxy::UpdateMaterials(const std::vector<MaterialParamSnapshot>& InSnapshots, RenderContext& InContext)
	{
		mMaterialProxies.resize(InSnapshots.size());
        for (const auto& Snapshot : InSnapshots)
        {
            if (Snapshot.Source) TrackResource(Snapshot.Source);
            for (const auto& Pair : Snapshot.OverridedTextures) if (Pair.second) TrackResource(Pair.second);
        }
		for (size_t i = 0; i < InSnapshots.size(); ++i)
		{
			mMaterialProxies[i].UpdateFromSnapshot(InSnapshots[i], InContext);
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
		if (!InRenderQueue || !IsVisible() || !IsBindingsInitialized())
			return;

		const auto& Buffers = mMeshData.VertexBuffers;
		if (!mMeshData.IndexBuffer || Buffers.empty() || Buffers.size() > STREAM_INPUT_MAX ||
			Buffers.size() != mMeshData.Layout.Buffers.size() ||
			(mMeshData.IndexStride != 2 && mMeshData.IndexStride != 4))
			return;

		RHI_DRAW_RESOURCE_BINDING Geometry;
		uint32 VertexCount = 0;
		for (size_t i = 0; i < Buffers.size(); ++i)
		{
			const auto& Layout = mMeshData.Layout.Buffers[i];
			if (!Buffers[i] || !Layout.Stride || Layout.Rate != PerVertex ||
				Buffers[i]->GetDesc().Size % Layout.Stride != 0)
				return;
			const uint32 Count = Buffers[i]->GetDesc().Size / Layout.Stride;
			if (!Count || (i && Count != VertexCount))
				return;
			VertexCount = Count;
			Geometry.mDrawData.mStreamInput.mIStreamBuffer[i] = Buffers[i];
		}
		Geometry.mIndexBuffer = mMeshData.IndexBuffer;
		Geometry.mIndexType = mMeshData.IndexStride == 2 ? INDEX_TYPE_UINT16 : INDEX_TYPE_UINT32;
		Geometry.mVertexCount = VertexCount;
		const uint32 AvailableIndices = mMeshData.IndexBuffer->GetDesc().Size / mMeshData.IndexStride;

		for (const auto& Sub : mMeshData.SubMeshes)
		{
			if (!Sub.IndexCount || Sub.IndexCount % 3 || Sub.StartIndex > AvailableIndices ||
				Sub.IndexCount > AvailableIndices - Sub.StartIndex)
				continue;
			auto* Material = GetMaterialProxy(Sub.MaterialSlot);
			if (!Material || !Material->IsReady())
				continue;

			RHI_DRAW_RESOURCE_BINDING Binding = Geometry;
			Binding.mFirstIndex = Sub.StartIndex;
			Binding.mIndexCount = Sub.IndexCount;
			Binding.mVertexOffset = Sub.BaseVertex;
			Material->BindToResourceBinding(Binding);

			for (const auto& Pass : Material->GetPasses())
			{
				if (!Pass.GetPipelineRHI())
					continue;
				const auto& QueueName = Pass.GetRenderQueueName();
				NamedRenderQueue Queue;
				if (QueueName == "shadow" || Pass.GetPassName() == Name("Shadow") || Pass.GetPassName() == Name("ShadowPass"))
				{
					if (!mbCastShadow) continue;
					Queue = RenderQueue_Shadow;
				}
				else if (QueueName == "opaque") Queue = RenderQueue_Normal;
				else if (QueueName == "transparent") Queue = RenderQueue_Transparent;
				else if (QueueName == "background" || QueueName == "sky") Queue = RenderQueue_Sky;
				else if (QueueName == "screen" || QueueName == "overlay") Queue = RenderQueue_Screen;
				else if (QueueName == "ui") Queue = RenderQueue_UI;
				else continue;

				if (!mObjectUniformBuffer || mUploadedWorldMatrix != mWorldMatrix)
				{
					struct ObjectUniformData
					{
						Matrix4x4 Model;
						Matrix4x4 Normal;
					} Data;
					Data.Model = mWorldMatrix;
					Data.Normal = std::abs(mWorldMatrix.determinant()) > 1e-8f ?
						mWorldMatrix.inverse().transpose() : Matrix4x4::IDENTITY;
					RHIUniformBufferDesc Desc;
					Desc.Size = sizeof(Data);
					Desc.InitialData = &Data;
					Desc.Slot = RHIUniformSlot::PerObject;
					// 新建不可变的变换数据，已入队的绘制继续持有旧缓冲。
					mObjectUniformBuffer = GetDynamicRHI()->GetDefaultCommandContext()->RHICreateUniformBufferWithSlot(Desc);
					if (!mObjectUniformBuffer) return;
					mUploadedWorldMatrix = mWorldMatrix;
				}
				Binding.SetVSUniformBuffer(0, mObjectUniformBuffer.get());
				InRenderQueue->GetRenderQueue(Queue)->UpdateRenderQueue(Pass.GetPipelineRHI(), Binding, mRenderLayer);
			}
		}
	}

}
