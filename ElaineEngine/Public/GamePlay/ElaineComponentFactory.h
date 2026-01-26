#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineSingleton.h"
#include "ElaineName.h"

namespace Elaine
{
	class Component;
	class ComponentInfo;
	class GameObject;

	class ElaineEngineExport ComponentFactory
	{
	public:
		ComponentFactory(const char* InType);
		
		virtual ~ComponentFactory();
		Component* CreateComponent(GameObject* InObject);
		ComponentInfo* CreateComponentInfo();
		virtual Component*				CreateComponentImpl(GameObject* InObject) = 0;
		virtual ComponentInfo*			CreateComponentInfoImpl() = 0;
		void							DestoryComponent(Component* InComponent);
		void							DestoryComponentInfo(ComponentInfo* InInfo);
		void							DestoryAllComponent();
	protected:
		Name mType;
		std::set<Component*> mComponents;
		std::set<ComponentInfo*> mComponentInfos;
	};

	class ElaineEngineExport ComponentFactoryManager :public Singleton<ComponentFactoryManager>
	{
	public:
		ComponentFactoryManager() = default;
		~ComponentFactoryManager();
		Component* CreateComponent(const Name& InType, GameObject* InObject);
		ComponentInfo* CreateComponentInfo(const Name& InType);

		ComponentFactory* GetComponentFactory(const Name& InType);
		void RegisterFactory(const Name& InType, ComponentFactory* InFactory);
	private:
		std::unordered_map<Name, ComponentFactory*> mFactoryMap;
	};
}