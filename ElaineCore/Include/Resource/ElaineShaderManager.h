#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceManager.h"
#include "ElaineShader.h"

namespace Elaine
{
	class ElaineCoreExport ShaderManager : public ResourceManager, public Singleton<ShaderManager>
	{
	public:
		ShaderManager();
		virtual ~ShaderManager() override;
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}