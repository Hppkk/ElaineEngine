#include <ElainePrecompiledHeader.h>
#include <ElaineModuleManager.h>
#include <ElaineModuleBase.h>

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
		//file open ModuleSupport.txt
		//RegisterModule()
	}

	void ModuleManager::RegisterModule(ModuleFeatures InFeature, ModuleBase* InModule)
	{
		if (InModule == nullptr)
			return;

		mModuleFeatures.emplace(InFeature, InModule);
	}

	void ModuleManager::UnregisterModule(ModuleFeatures InFeature)
	{
		auto Iter = mModuleFeatures.find(InFeature);
		if (Iter != mModuleFeatures.end())
		{
			Iter->second->Terminate();
			SAFE_DELETE(Iter->second);
		}
	}
}