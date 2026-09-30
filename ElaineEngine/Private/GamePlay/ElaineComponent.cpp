#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineComponent.h"
#include "ElaineWorld.h"

namespace Elaine
{
	ActorComponentInfo::ActorComponentInfo()
	{

	}

	ActorComponentInfo::~ActorComponentInfo()
	{

	}

	void ActorComponentInfo::ExportData(JsonCpp& InJson)
	{
		if (InJson.empty())
			return;

		ExportDataImpl(InJson);
	}

	void ActorComponentInfo::ImportData(const JsonCpp& InJson)
	{
		if (InJson.empty())
			return;

		ImportDataImpl(InJson);
	}

	void ActorComponentInfo::ExportDataImpl(JsonCpp& InJson)
	{
	}

	void ActorComponentInfo::ImportDataImpl(const JsonCpp& InJson)
	{
	}

	ActorComponent::ActorComponent(Actor* InObject)
		:mParent(InObject)
	{

	}
	ActorComponent::~ActorComponent()
	{
		
	}

	void ActorComponent::Initialize(ActorComponentInfo* info)
	{

	}

	void ActorComponent::OnRegisterWorld(World* InWorld)
	{
		mWorld = InWorld;
		OnRegisterWorldImpl(InWorld);
	}

	void ActorComponent::OnUnregisterWorld()
	{
		OnUnregisterWorldImpl();
		mWorld = nullptr;
	}
	void ActorComponent::SetVisible(bool InVisible)
	{
	}
}