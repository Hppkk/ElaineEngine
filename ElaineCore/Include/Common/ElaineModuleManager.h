#pragma once

namespace Elaine
{
	class ModuleBase;

	enum ModuleFeature
	{
		MF_RenderModule = 1,
		MF_SoundModule = 1 << 1,
		MF_PhysicsModule = 1 << 2,
		MF_AnimationModule = 1 << 3,
		MF_ResourceModule = 1 << 4,
	};

	class ElaineCoreExport ModuleManager : public Singleton<ModuleManager>
	{
	public:
		ModuleManager() = default;
		~ModuleManager();
		void Initialize();
		void RegisterModule(ModuleFeature InFeature, ModuleBase* InModule);
		void UnregisterModule(ModuleFeature InFeature);
		template<typename T>
		T* GetModule(ModuleFeature InFeature)
		{
			return static_cast<T*>(mModuleFeatures[InFeature]);
		}
	private:
		std::map<ModuleFeature, ModuleBase*> mModuleFeatures;
	};

#ifndef GETMODULE
#define GETMODULE(MODULE_NAME) ModuleManager::instance()->GetModule<MODULE_NAME>(MF_##MODULE_NAME)
#endif
}