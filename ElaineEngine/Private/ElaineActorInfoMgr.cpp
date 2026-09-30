#include "ElainePrecompiledHeader.h"
#include "ElaineActorInfoMgr.h"
#include "GamePlay/ElaineActor.h"

namespace Elaine
{
	ActorInfoMgr::ActorInfoMgr()
		: ResourceManager(RT_Actor)
	{
		
	}

	ActorInfoMgr::~ActorInfoMgr()
	{

	}

	ResourceBasePtr Elaine::ActorInfoMgr::CreateResourceImpl(const std::string& InPath)
	{
		return ResourcePtr<ActorInfo>(new ActorInfo(this, InPath));
	}
}