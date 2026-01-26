#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineComponentFactory.h"
#include "GamePlay/ElaineComponent.h"

namespace Elaine
{
	void ComponentFactory::DestoryComponent(Component* InComponent)
	{
		if (InComponent == nullptr)
			return;

		auto Iter = mComponents.find(InComponent);
		if (Iter == mComponents.end())
			return;

		mComponents.erase(Iter);
		
		SAFE_DELETE(InComponent)
	}

	void ComponentFactory::DestoryComponentInfo(ComponentInfo* InInfo)
	{
		if (InInfo == nullptr)
			return;

		auto Iter = mComponentInfos.find(InInfo);
		if (Iter == mComponentInfos.end())
			return;

		mComponentInfos.erase(Iter);

		SAFE_DELETE(InInfo)
	}

	void ComponentFactory::DestoryAllComponent()
	{

	}

	ComponentFactory::ComponentFactory(const char* InType)
	{
		mType = InType;
	}

	ComponentFactory::~ComponentFactory()
	{
		for (auto info : mComponentInfos)
		{
			SAFE_DELETE(info)
		}

		for (auto com : mComponents)
		{
			SAFE_DELETE(com);
		}

		mComponents.clear();
		mComponentInfos.clear();
	}

	Component* ComponentFactory::CreateComponent(GameObject* InObject)
	{
		Component* NewComponent = CreateComponentImpl(InObject);
		NewComponent->OnCreate();
		mComponents.insert(NewComponent);
		return NewComponent;
	}

	ComponentInfo* ComponentFactory::CreateComponentInfo()
	{
		ComponentInfo* NewComInfo = CreateComponentInfoImpl();
		mComponentInfos.insert(NewComInfo);
		return NewComInfo;
	}

	ComponentFactoryManager::~ComponentFactoryManager()
	{
		for (auto&& CFactory : mFactoryMap)
		{
			SAFE_DELETE(CFactory.second);
		}
		mFactoryMap.clear();
	}

	Component* ComponentFactoryManager::CreateComponent(const Name& InType, GameObject* InObject)
	{
		ComponentFactory* ComFactory = GetComponentFactory(InType);
		if (ComFactory != nullptr)
		{
			return ComFactory->CreateComponent(InObject);
		}
		return nullptr;
	}

	ComponentInfo* ComponentFactoryManager::CreateComponentInfo(const Name& InType)
	{
		ComponentFactory* ComFactory = GetComponentFactory(InType);
		if (ComFactory != nullptr)
		{
			return ComFactory->CreateComponentInfo();
		}
		return nullptr;
	}

	ComponentFactory* Elaine::ComponentFactoryManager::GetComponentFactory(const Name& InType)
	{
		auto Iter = mFactoryMap.find(InType);
		if (Iter == mFactoryMap.end())
			return nullptr;

		return Iter->second;
	}

	void Elaine::ComponentFactoryManager::RegisterFactory(const Name& InType, ComponentFactory* InFactory)
	{
		auto Iter = mFactoryMap.find(InType);
		if (Iter != mFactoryMap.end())
		{
			LOG_FATAL("This component factory has already been registered.");
			return;
		}

		mFactoryMap[InType] = InFactory;
	}

}