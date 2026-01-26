#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineComponent.h"
#include "ElaineWorld.h"

namespace Elaine
{
	ComponentInfo::ComponentInfo()
	{

	}

	ComponentInfo::~ComponentInfo()
	{

	}

	void ComponentInfo::ExportData(JsonCpp& InJson)
	{
		if (InJson.empty())
			return;

		ExportDataImpl(InJson);
	}

	void ComponentInfo::ImportData(const JsonCpp& InJson)
	{
		if (InJson.empty())
			return;

		ImportDataImpl(InJson);
	}

	void ComponentInfo::ExportDataImpl(JsonCpp& InJson)
	{
	}

	void ComponentInfo::ImportDataImpl(const JsonCpp& InJson)
	{
	}

	Component::Component(GameObject* InObject)
		:mParent(InObject)
	{

	}
	Component::~Component()
	{
		
	}

	void Component::Initialize(ComponentInfo* info)
	{

	}

	void Component::OnRegisterWorld(World* InWorld)
	{
		mWorld = InWorld;
		OnRegisterWorldImpl(InWorld);
	}

	void Component::OnUnregisterWorld()
	{
		OnUnregisterWorldImpl();
		mWorld = nullptr;
	}
	void Component::SetVisible(bool InVisible)
	{
	}
}