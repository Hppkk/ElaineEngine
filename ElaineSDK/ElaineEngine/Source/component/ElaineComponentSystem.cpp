#include "component/ElaineComponentSystem.h"
#include "component/ElaineComponentFactory.h"
#include "component/ElaineTransformComponent.h"
#include "component/ElaineComponent.h"
#include "ElaineCoreMacroDefinition.h"

namespace Elaine
{
	ComponentSystem::ComponentSystem()
	{
		new ComponentFactoryManager();
		new ComponentTickManager();
		registerComFactory();
	}

	ComponentSystem::~ComponentSystem()
	{
		delete ComponentFactoryManager::instance();
		delete ComponentTickManager::instance();
	}

	void ComponentSystem::registerComFactory()
	{
		REGISTERCOMFACTORY(TransformComponent);
	}
}