#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineRVTVolumeComponent.h"
#include "ElaineRenderCommandQueue.h"
#include "RenderProxy/ElaineRVTRenderProxy.h"
#include "ElaineGameObject.h"

namespace Elaine
{
	RVTVolumeComponent::RVTVolumeComponent(GameObject* InObject)
		: Component(InObject)
	{
	}

	RVTVolumeComponent::~RVTVolumeComponent()
	{
	}

	const Name& RVTVolumeComponent::GetType() const
	{
		static Name NameType("RVTVolumeComponent");
		return NameType;
	}

	//=========================================================================
	// OnRegisterWorldImpl — 模式 A：在渲染线程创建 RenderProxy
	// 遵循 StaticMeshComponent::OnRegisterWorldImpl 的模式
	//=========================================================================
	void RVTVolumeComponent::OnRegisterWorldImpl(World* InWorld)
	{
		// 在逻辑线程构建配置快照（值拷贝）
		RVTRenderProxy::Config ConfigSnapshot;
		ConfigSnapshot.Name = mVolumeName;
		ConfigSnapshot.WorldMinX = mWorldMinX;
		ConfigSnapshot.WorldMinY = mWorldMinY;
		ConfigSnapshot.WorldMaxX = mWorldMaxX;
		ConfigSnapshot.WorldMaxY = mWorldMaxY;
		ConfigSnapshot.VirtualSizeX = mVirtualSizeX;
		ConfigSnapshot.VirtualSizeY = mVirtualSizeY;
		ConfigSnapshot.NumLayers = mNumLayers;
		ConfigSnapshot.NumMipLevels = mNumMipLevels;
		ConfigSnapshot.MaxTileRendersPerFrame = mMaxTileRendersPerFrame;

		ENQUEUE_RENDER_COMMAND(CreateRVTProxy)(
			[this, ConfigSnapshot](RenderContext& Context)
			{
				// 在渲染线程执行：创建 Proxy 并初始化 RVT 资源
				SceneManager* SceneMgr = GetGameObject()->GetSceneManager();
				mRenderProxy = static_cast<RVTRenderProxy*>(
					SceneMgr->CreateRenderProxy(EProxyType::RVTVolume));

				if (mRenderProxy)
				{
					mRenderProxy->InitializeRVT(ConfigSnapshot, SceneMgr);
					mRenderProxy->mUserData = GetGameObject();
					mRenderProxy->mUserType = 1; // 1 = GameObject
				}
			});
	}

	//=========================================================================
	// OnUnregisterWorldImpl — 模式 C：在渲染线程销毁 RenderProxy
	// 遵循 StaticMeshComponent::OnUnregisterWorldImpl 的模式
	//=========================================================================
	void RVTVolumeComponent::OnUnregisterWorldImpl()
	{
		if (mRenderProxy)
		{
			RVTRenderProxy* ProxyToDestroy = mRenderProxy;
			mRenderProxy = nullptr;  // 逻辑线程立即断开引用

			ENQUEUE_RENDER_COMMAND(DestroyRVTProxy)(
				[ProxyToDestroy, this](RenderContext& Context)
				{
					// 在渲染线程执行：先 shutdown RVT 资源，再销毁 Proxy
					ProxyToDestroy->ShutdownRVT();
					GetGameObject()->GetSceneManager()->DestroyRenderProxy(ProxyToDestroy);
				});
		}
	}

	//=========================================================================
	// SetWorldBounds — 模式 B：值拷贝 + ENQUEUE 更新
	//=========================================================================
	void RVTVolumeComponent::SetWorldBounds(float MinX, float MinY, float MaxX, float MaxY)
	{
		// 更新逻辑线程的数据
		mWorldMinX = MinX;
		mWorldMinY = MinY;
		mWorldMaxX = MaxX;
		mWorldMaxY = MaxY;

		if (mRenderProxy == nullptr)
			return;

		// 值拷贝传递到渲染线程
		RVTRenderProxy* Proxy = mRenderProxy;
		ENQUEUE_RENDER_COMMAND(UpdateRVTBounds)(
			[Proxy, MinX, MinY, MaxX, MaxY](RenderContext& Context)
			{
				Proxy->SetWorldBounds(MinX, MinY, MaxX, MaxY);
			});
	}

	void RVTVolumeComponent::GetWorldBounds(float& OutMinX, float& OutMinY,
		float& OutMaxX, float& OutMaxY) const
	{
		OutMinX = mWorldMinX;
		OutMinY = mWorldMinY;
		OutMaxX = mWorldMaxX;
		OutMaxY = mWorldMaxY;
	}

	//=========================================================================
	// InvalidateRegion — 值拷贝 + ENQUEUE
	//=========================================================================
	void RVTVolumeComponent::InvalidateRegion(float MinX, float MinY, float MaxX, float MaxY)
	{
		if (mRenderProxy == nullptr)
			return;

		RVTRenderProxy* Proxy = mRenderProxy;
		ENQUEUE_RENDER_COMMAND(InvalidateRVTRegion)(
			[Proxy, MinX, MinY, MaxX, MaxY](RenderContext& Context)
			{
				Proxy->InvalidateRegion(MinX, MinY, MaxX, MaxY);
			});
	}

	void RVTVolumeComponent::InvalidateAll()
	{
		if (mRenderProxy == nullptr)
			return;

		RVTRenderProxy* Proxy = mRenderProxy;
		ENQUEUE_RENDER_COMMAND(InvalidateRVTAll)(
			[Proxy](RenderContext& Context)
			{
				Proxy->InvalidateAll();
			});
	}

	void RVTVolumeComponent::SetVolumeName(const std::string& InName)
	{
		mVolumeName = InName;
		MarkRenderStateDirty();
	}

	void RVTVolumeComponent::SetResolution(uint32 SizeX, uint32 SizeY)
	{
		mVirtualSizeX = SizeX;
		mVirtualSizeY = SizeY;
		// Resolution change requires full RVT rebuild, handled by MarkRenderStateDirty
		MarkRenderStateDirty();
	}

	void RVTVolumeComponent::SetNumLayers(uint8 InNumLayers)
	{
		mNumLayers = InNumLayers;
		MarkRenderStateDirty();
	}

	void RVTVolumeComponent::SetMaxTileRendersPerFrame(uint32 InMax)
	{
		mMaxTileRendersPerFrame = InMax;
		// This is a soft parameter, no need to rebuild RVT
		// But still enqueue for consistency
		MarkRenderStateDirty();
	}

	//=========================================================================
	// MarkRenderStateDirty — 模式 B：完整配置快照 + ENQUEUE
	// 遵循 StaticMeshComponent::MarkRenderStateDirty 的模式
	//=========================================================================
	void RVTVolumeComponent::MarkRenderStateDirty()
	{
		if (mRenderProxy == nullptr)
			return;

		// 在逻辑线程构建配置快照（值拷贝，无竞态）
		RVTRenderProxy* Proxy = mRenderProxy;
		RVTRenderProxy::Config ConfigSnapshot;
		ConfigSnapshot.Name = mVolumeName;
		ConfigSnapshot.WorldMinX = mWorldMinX;
		ConfigSnapshot.WorldMinY = mWorldMinY;
		ConfigSnapshot.WorldMaxX = mWorldMaxX;
		ConfigSnapshot.WorldMaxY = mWorldMaxY;
		ConfigSnapshot.VirtualSizeX = mVirtualSizeX;
		ConfigSnapshot.VirtualSizeY = mVirtualSizeY;
		ConfigSnapshot.NumLayers = mNumLayers;
		ConfigSnapshot.NumMipLevels = mNumMipLevels;
		ConfigSnapshot.MaxTileRendersPerFrame = mMaxTileRendersPerFrame;

		ENQUEUE_RENDER_COMMAND(UpdateRVTConfig)(
			[Proxy, ConfigSnapshot](RenderContext& Context)
			{
				// 在渲染线程执行：UpdateConfig 会自动判断是否需要 rebuild
				// 如果分辨率/层数变更 → shutdown + re-init（保留 SceneManager 引用）
				// 如果只是 bounds/soft 参数变更 → 轻量更新
				Proxy->UpdateConfig(ConfigSnapshot);
			});
	}

} // namespace Elaine
