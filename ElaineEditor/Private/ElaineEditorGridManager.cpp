#include "ElaineEditorGridManager.h"
#include "ElaineWorld.h"
#include "ElaineSceneManager.h"
#include "ElaineRenderCommandQueue.h"
#include "RenderProxy/ElaineGridRenderProxy.h"
#include "ElaineMaterialInstanceDynamic.h"

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

		// Create the GridRenderProxy on the render thread.
		// SceneManager is created on the render thread in the World constructor,
		// so we must access it inside a render command to guarantee it exists.
		Elaine::MaterialInstanceDynamic* MaterialCopy = mMaterial;
		Elaine::World* WorldCopy = InWorld;

		ENQUEUE_RENDER_COMMAND(CreateGridRenderProxy)([WorldCopy, MaterialCopy](Elaine::RenderContext& InContext)
		{
			Elaine::SceneManager* SceneMgr = WorldCopy->GetSceneManager();
			if (!SceneMgr)
				return;

			Elaine::RenderProxy* NewProxy = SceneMgr->CreateRenderProxy(Elaine::EProxyType::Grid);
			Elaine::GridRenderProxy* GridProxy = static_cast<Elaine::GridRenderProxy*>(NewProxy);
			if (GridProxy)
			{
				GridProxy->SetMaterial(MaterialCopy);

				// Track material resources and begin initialization
				if (MaterialCopy && !MaterialCopy->GetSourceMaterial().isNull())
				{
					GridProxy->TrackResource(MaterialCopy->GetSourceMaterial());
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

		// Material cleanup
		delete mMaterial;
		mMaterial = nullptr;
		mInitialized = false;
	}
}
