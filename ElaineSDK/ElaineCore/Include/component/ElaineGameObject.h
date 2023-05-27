#pragma once
#include "ElaineCorePrerequirements.h"


/*----------------------------------------------
Engine Component Architecture: one gameobject each type of component can only hold one.
*///--------------------------------------------

namespace Elaine
{
	class ElaineCoreExport EGameObjectInfo
	{
	public:
		EGameObjectInfo();
		~EGameObjectInfo();
	};

	class EComponent;
	class SceneNode;
	class ElaineCoreExport EGameObject
	{
	public:
		EGameObject();
		~EGameObject();
		void									init();
		EComponent*								GetComponentByName(const std::string& name);
		std::vector<EComponent*>&				GetComponents() { return m_components; }
		void									AddChildGameObject(EGameObject* obj);
		void									AddComponents(EComponent* com);
		EGameObject*							CreateChildGameObject();

		template<typename ComponentType>
		ComponentType* GetComponent(const std::string& comType)
		{
			auto it = m_componentsMap.find(comType);
			if (it == m_componentsMap.end())
				return nullptr;
			return static_cast<ComponentType*>(*it);
		}

		Vector3									getWorldPosition() const { return m_WorldPosition; }
		Vector3									getWorldScale() const { return m_WorldScale; }
		Vector3									getWorldRotation() const { return m_WorldRotation; }
		EGameObject*							getParent() { return m_pParentGObject; }
		void									setWorldPosition(const Vector3& pos);
		void									setWorldScale(const Vector3& scale);
		void									setWorldRotation(const Vector3& rotation);
		void									updateNode(bool childUpdate = true);

	private:
		std::vector<EComponent*>						m_components;
		std::map<std::string, EComponent*>				m_componentsMap; //组件类型名，对应组件指针
		std::vector<EGameObject*>						m_childGameObjects;
		std::map<std::string, EGameObject*>				m_childGameObjectMap;
		EGameObjectInfo*								m_info = nullptr;
		SceneNode*										m_pSceneNode = nullptr;
		EGameObject*									m_pParentGObject = nullptr;


		Vector3											m_WorldPosition;
		Vector3											m_WorldScale;
		Vector3											m_WorldRotation;
	};
}
