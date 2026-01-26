#include <ElainePrecompiledHeader.h>
#include <ElaineModuleManager.h>
#include <ElaineModuleBase.h>
#include <ElaineResourceModule.h>
#include <common/ElaineRenderModule.h>

namespace Elaine
{
	ModuleManager::~ModuleManager()
	{
		auto TempModuleFeatures = mModuleFeatures;

		for (auto&& mod : TempModuleFeatures)
		{
			UnregisterModule(mod.first);
		}
		mModuleFeatures.clear();
	}

	void ModuleManager::Initialize()
	{
		//TODO : file open ModuleSupport.txt
		RegisterModule(MF_ResourceModule, new ResourceModule());
		RegisterModule(MF_RenderModule, new RenderModule());
	}

	void ModuleManager::RegisterModule(ModuleFeature InFeature, ModuleBase* InModule)
	{
		if (InModule == nullptr)
			return;

		mModuleFeatures.emplace(InFeature, InModule);
		InModule->Initialize();
	}

	void ModuleManager::UnregisterModule(ModuleFeature InFeature)
	{
		auto Iter = mModuleFeatures.find(InFeature);
		if (Iter != mModuleFeatures.end())
		{
			Iter->second->Terminate();
			SAFE_DELETE(Iter->second);
		}
	}
}