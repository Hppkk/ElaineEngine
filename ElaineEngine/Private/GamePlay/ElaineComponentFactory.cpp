#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineComponentFactory.h"
#include "GamePlay/ElaineComponent.h"

namespace Elaine
{
	void ComponentFactory::DestoryComponent(ActorComponent* InComponent)
	{
		if (InComponent == nullptr)
			return;

		auto Iter = mComponents.find(InComponent);
		if (Iter == mComponents.end())
			return;

		mComponents.erase(Iter);
		
		SAFE_DELETE(InComponent)
	}

	void ComponentFactory::DestoryActorComponentInfo(ActorComponentInfo* InInfo)
	{
		if (InInfo == nullptr)
			return;

		auto Iter = mActorComponentInfos.find(InInfo);
		if (Iter == mActorComponentInfos.end())
			return;

		mActorComponentInfos.erase(Iter);

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
		for (auto info : mActorComponentInfos)
		{
			SAFE_DELETE(info)
		}

		for (auto com : mComponents)
		{
			SAFE_DELETE(com);
		}

		mComponents.clear();
		mActorComponentInfos.clear();
	}

	ActorComponent* ComponentFactory::CreateComponent(Actor* InObject)
	{
		ActorComponent* NewComponent = CreateComponentImpl(InObject);
		NewComponent->OnCreate();
		mComponents.insert(NewComponent);
		return NewComponent;
	}

	ActorComponentInfo* ComponentFactory::CreateActorComponentInfo()
	{
		ActorComponentInfo* NewComInfo = CreateActorComponentInfoImpl();
		mActorComponentInfos.insert(NewComInfo);
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

	ActorComponent* ComponentFactoryManager::CreateComponent(const Name& InType, Actor* InObject)
	{
		ComponentFactory* ComFactory = GetComponentFactory(InType);
		if (ComFactory != nullptr)
		{
			return ComFactory->CreateComponent(InObject);
		}
		return nullptr;
	}

	ActorComponentInfo* ComponentFactoryManager::CreateActorComponentInfo(const Name& InType)
	{
		ComponentFactory* ComFactory = GetComponentFactory(InType);
		if (ComFactory != nullptr)
		{
			return ComFactory->CreateActorComponentInfo();
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
			LOG_FATAL("This ActorComponent factory has already been registered.");
			return;
		}

		mFactoryMap[InType] = InFactory;
	}

}