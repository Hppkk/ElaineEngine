#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"

namespace Elaine
{
	class ShaderPass;

	class ElaineCoreExport ShaderPassManager : public Singleton<ShaderPassManager>
	{
	public:
		ShaderPassManager();
		~ShaderPassManager();

		bool ParseShaderPass(const JsonCpp& InJson);
		void UnloadShaderPass(ShaderPass* InShaderPass, JsonCpp& OutJson);
		void CalculatePassStateHash(ShaderPass* InShaderPass);
		ShaderPass* CreateShaderPass(const JsonCpp& InJson);
		bool SaveShaderPass(ShaderPass* InShaderPass);
		ShaderPass* GetShaderPass(const Name& InName) const;
		void CompilePipeline(ShaderPass* InShaderPass);
		void DestroyShaderPass(ShaderPass* InShaderPass);
	private:
		std::unordered_map<uint64_t, ShaderPass*> mShaderPasses;
	};
}