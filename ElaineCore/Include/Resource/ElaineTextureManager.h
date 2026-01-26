#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceManager.h"
#include "ElaineTextureResource.h"

namespace Elaine
{
	class ElaineCoreExport TextureManager : public ResourceManager, public Singleton<TextureManager>
	{
	public:
		TextureManager();
		virtual ~TextureManager() override;
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}