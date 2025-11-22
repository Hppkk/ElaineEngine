#pragma once

namespace Elaine
{
	class ModuleBase;

	enum ModuleFeatures
	{
		MF_RenderModule = 1,
		MF_SoundModule = 1 << 1,
		MF_PhysicsModule = 1 << 2,
		MF_AnimationModule = 1 << 3,
	};

	class ElaineCoreExport ModuleManager : public Singleton<ModuleManager>
	{
	public:
		ModuleManager() = default;
		~ModuleManager();
		void Initialize();
		void RegisterModule(ModuleFeatures InFeature, ModuleBase* InModule);
		void UnregisterModule(ModuleFeatures InFeature);
	private:
		std::map<ModuleFeatures, ModuleBase*> mModuleFeatures;
	};
}