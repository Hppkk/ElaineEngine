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
        InJson = mSerializedJson.is_object() ? mSerializedJson : JsonCpp::object();
        InJson["Type"] = mType.ToString();
        ExportDataImpl(InJson);
	}

	void ActorComponentInfo::ImportData(const JsonCpp& InJson)
	{
		if (InJson.empty())
			return;

        mSerializedJson = InJson;
        if (InJson.contains("Type")) mType = Name(InJson["Type"].get<std::string>().c_str());
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
        mbVisible = InVisible;
        ApplyVisibility();
    }
    void ActorComponent::PostEditChangeProperty(const PropertyChangedEvent& Event)
    {
        if (Event.Type == PropertyChangeType::EditFinished) return;
        if (Event.IsProperty("mbVisible")) ApplyVisibility();
    }
}