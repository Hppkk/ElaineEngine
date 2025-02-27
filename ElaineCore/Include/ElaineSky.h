#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineMaterial.h"

namespace Elaine
{
	class ElaineCoreExport SkyObject : public RenderableObject
	{
	public:
		SkyObject(SceneNode* InParent, SceneManager* InSceneMgr);
		~SkyObject();
		void InitilizeMaterial();
		virtual Material* GetMaterial() override { return mMaterial; }
		virtual void SynchRenderData() override;
		virtual void NotifyCurrentCamera(Camera* InCamera) override;
		virtual void UpdateRenderQueue(RenderQueueSet* InRenderQueueSet) override;
		virtual void RecordRenderCommand(RHICommandList* InRHICommandList) override;
	private:
		Material* mMaterial = nullptr;
	};
}