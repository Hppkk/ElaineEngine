#include "ElaineEditorGridManager.h"
#include "ElaineWorld.h"
#include "ElaineSceneManager.h"
#include "ElaineRenderCommandQueue.h"
#include "RenderProxy/ElaineGridRenderProxy.h"
#include "ElaineMaterialInstanceDynamic.h"
#include "ElaineMaterialParamSnapshot.h"

namespace Editor
{
	EditorGridManager::~EditorGridManager()
	{
		Shutdown();
	}

	void EditorGridManager::Initialize(Elaine::World* InWorld)
	{
		if (mInitialized || !InWorld)
			return;

		// Create grid material instance (on the logic thread)
		mMaterial = new Elaine::MaterialInstanceDynamic();
		mWorld = InWorld;
		mState = std::make_shared<ProxyState>();
		mState->Visible.store(mVisible);
		mMaterial->ChangeMaterial("material_instance/Grid.mi");

		// 在逻辑线程生成材质参数快照（不含 RHI 资源）
		Elaine::MaterialParamSnapshot Snapshot = mMaterial->CreateSnapshot();
		Elaine::World* WorldCopy = InWorld;
		auto State = mState;

		ENQUEUE_RENDER_COMMAND(CreateGridRenderProxy)([State, WorldCopy, Snapshot = std::move(Snapshot)](Elaine::RenderContext& InContext)
		{
			Elaine::SceneManager* SceneMgr = WorldCopy->GetSceneManager();
			if (!SceneMgr)
				return;

			Elaine::RenderProxy* NewProxy = SceneMgr->CreateRenderProxy(Elaine::EProxyType::Grid);
			Elaine::GridRenderProxy* GridProxy = static_cast<Elaine::GridRenderProxy*>(NewProxy);
			if (GridProxy)
			{
				if (!State->Active.load())
				{
					SceneMgr->DestroyRenderProxy(GridProxy);
					return;
				}
				State->Proxy.store(GridProxy);
				GridProxy->SetVisible(State->Visible.load());
				// 用快照更新渲染线程的 RenderMaterialProxy
				GridProxy->UpdateMaterial(Snapshot);

				// Track material resources and begin initialization
				if (Snapshot.IsValid())
				{
					GridProxy->TrackResource(Snapshot.Source);
				}
				GridProxy->BeginInitialization();
			}
		});

		mInitialized = true;
	}

	void EditorGridManager::Shutdown()
	{
		if (!mInitialized)
			return;

		auto State = mState;
		State->Active.store(false);
		Elaine::World* World = mWorld;
		if (World)
		{
			ENQUEUE_RENDER_COMMAND(DestroyEditorGridProxy)([State, World](Elaine::RenderContext&)
			{
				Elaine::GridRenderProxy* Proxy = State->Proxy.exchange(nullptr);
				if (Proxy && World->GetSceneManager())
					World->GetSceneManager()->DestroyRenderProxy(Proxy);
			});
		}

		// Material cleanup (logic thread ownership)
		delete mMaterial;
		mMaterial = nullptr;
		mWorld = nullptr;
		mState.reset();
		mInitialized = false;
	}

	void EditorGridManager::SetVisible(bool Visible)
	{
		if (mVisible == Visible)
			return;
		mVisible = Visible;
		auto State = mState;
		if (State)
		{
			State->Visible.store(Visible);
			ENQUEUE_RENDER_COMMAND(SetEditorGridVisibility)([State, Visible](Elaine::RenderContext&)
			{
				if (auto* Proxy = State->Proxy.load()) Proxy->SetVisible(Visible);
			});
		}
	}
}
