#include "component/ElaineComponentSystem.h"
#include "component/ElaineComponentFactory.h"

namespace Elaine
{
	ComponentSystem::ComponentSystem()
	{
		new ComponentFactoryManager();
		registerComFactory();
	}

	ComponentSystem::~ComponentSystem()
	{
		delete ComponentFactoryManager::instance();
	}

	void ComponentSystem::registerComFactory()
	{
		//REGISTERCOMFACTORY(TransformComponent);
	}
}