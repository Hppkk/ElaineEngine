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
		mMaterial->ChangeMaterial("material_instance/Grid.mi");

		// 在逻辑线程生成材质参数快照（不含 RHI 资源）
		Elaine::MaterialParamSnapshot Snapshot = mMaterial->CreateSnapshot();
		Elaine::World* WorldCopy = InWorld;

		ENQUEUE_RENDER_COMMAND(CreateGridRenderProxy)([WorldCopy, Snapshot = std::move(Snapshot)](Elaine::RenderContext& InContext)
		{
			Elaine::SceneManager* SceneMgr = WorldCopy->GetSceneManager();
			if (!SceneMgr)
				return;

			Elaine::RenderProxy* NewProxy = SceneMgr->CreateRenderProxy(Elaine::EProxyType::Grid);
			Elaine::GridRenderProxy* GridProxy = static_cast<Elaine::GridRenderProxy*>(NewProxy);
			if (GridProxy)
			{
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

		// Material cleanup (逻辑线程持有，逻辑线程销毁)
		delete mMaterial;
		mMaterial = nullptr;
		mInitialized = false;
	}
}