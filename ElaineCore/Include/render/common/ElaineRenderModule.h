#pragma once

namespace Elaine
{
	class DynamicRHI;

	// initilize render module

	class ElaineCoreExport RenderModule
	{
	public:
		RenderModule();
		~RenderModule();
		DynamicRHI* LoadDynamicRHI(const RHI_PARAM_DESC& InDesc);
	private:
		DynamicRHI* mDynamicRHI = nullptr;
		bool mbIsLoaded = false;
	};
}