#include "ElaineCoreMacroDefinition.h"
#include "GamePlay/ElaineComponent.h"
#include "GamePlay/ElaineComponentSystem.h"
#include "GamePlay/ElaineComponentFactory.h"
#include "GamePlay/ElaineTransformComponent.h"
#include "GamePlay/ElaineMeshComponent.h"
#include "GamePlay/ElaineSkyComponent.h"
#include "GamePlay/ElaineCameraComponent.h"

namespace Elaine
{
	ComponentSystem::ComponentSystem()
	{
		new ComponentFactoryManager();
		RegisterFactory();
	}

	ComponentSystem::~ComponentSystem()
	{
		delete ComponentFactoryManager::instance();
	}

	void ComponentSystem::RegisterFactory()
	{
		REGISTER_COM_FACTORY(TransformComponent);
		REGISTER_COM_FACTORY(StaticMeshComponent);
		REGISTER_COM_FACTORY(SkyComponent);
		REGISTER_COM_FACTORY(CameraComponent);
	}
}