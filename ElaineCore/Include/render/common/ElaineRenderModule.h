#pragma once
#include <ElaineModuleManager.h>
#include <ElaineModuleBase.h>

namespace Elaine
{
	class DynamicRHI;

	// initilize render module

	class ElaineCoreExport RenderModule : public ModuleBase
	{
	public:
		RenderModule();
		virtual ~RenderModule();
		void LoadDynamicRHI(const RHI_PARAM_DESC& InDesc);
		virtual void Initialize() override;
		virtual void Terminate() override;
	private:
		bool mbIsLoaded = false;
	};
}