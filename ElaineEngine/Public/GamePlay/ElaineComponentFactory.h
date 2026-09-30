#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineSingleton.h"
#include "ElaineName.h"

namespace Elaine
{
	class ActorComponent;
	class ActorComponentInfo;
	class Actor;

	class ElaineEngineExport ComponentFactory
	{
	public:
		ComponentFactory(const char* InType);
		
		virtual ~ComponentFactory();
		ActorComponent* CreateComponent(Actor* InObject);
		ActorComponentInfo* CreateActorComponentInfo();
		virtual ActorComponent*				CreateComponentImpl(Actor* InObject) = 0;
		virtual ActorComponentInfo*			CreateActorComponentInfoImpl() = 0;
		void							DestoryComponent(ActorComponent* InComponent);
		void							DestoryActorComponentInfo(ActorComponentInfo* InInfo);
		void							DestoryAllComponent();
	protected:
		Name mType;
		std::set<ActorComponent*> mComponents;
		std::set<ActorComponentInfo*> mActorComponentInfos;
	};

	class ElaineEngineExport ComponentFactoryManager :public Singleton<ComponentFactoryManager>
	{
	public:
		ComponentFactoryManager() = default;
		~ComponentFactoryManager();
		ActorComponent* CreateComponent(const Name& InType, Actor* InObject);
		ActorComponentInfo* CreateActorComponentInfo(const Name& InType);

		ComponentFactory* GetComponentFactory(const Name& InType);
		void RegisterFactory(const Name& InType, ComponentFactory* InFactory);
	private:
		std::unordered_map<Name, ComponentFactory*> mFactoryMap;
	};
}