#pragma once
#include "ElaineCorePrerequirements.h"
#include "resource/ElaineResourceBase.h"


/*----------------------------------------------
Engine Component Architecture: one gameobject each type of component can only hold one.
*///--------------------------------------------

namespace Elaine
{
	class ElaineCoreExport EGameObjectInfo :public ResourceBase
	{
		friend class EGameObject;
	public:
		EGameObjectInfo();
		EGameObjectInfo(const std::string& path);
		~EGameObjectInfo();
		virtual void			loadImpl() override;
		virtual	void			unloadImpl() override;
	private:
		std::string					m_sFilePath;
		std::set<EGameObject*>		m_InstGameObjectSet;
	};

	class EComponent;
	class SceneNode;
	class ElaineCoreExport EGameObject
	{
		friend class EGameObjectMgr;
	public:
		EGameObject();
		EGameObject(const std::string& name);
		~EGameObject();
		void									init(EGameObjectInfo* info);
		EComponent*								GetComponentByName(const std::string& name);
		std::vector<EComponent*>&				GetComponents() { return m_components; }
		void									AddChildGameObject(EGameObject* obj);
		void									AddComponents(EComponent* com);
		EGameObject*							CreateChildGameObject();
		void									destroy();
		void									removeComponent(EComponent* rhs);
		void									removeChildGameObject(EGameObject* rhs);
		template<typename ComponentType>
		ComponentType* GetComponent()
		{
			for (auto& com : m_components)
			{
				if(com->m_sType==ComponentType::m_sType)
					return static_cast<ComponentType*>(com);
			}
			return nullptr;
			
		}

		Vector3									getWorldPosition() const { return m_WorldPosition; }
		Vector3									getWorldScale() const { return m_WorldScale; }
		Vector3									getWorldRotation() const { return m_WorldRotation; }
		Quaternion								getWorldQuaternion() const { return m_WorldQuaternion; }

		EGameObject*							getParent() { return m_pParentGObject; }
		void									setWorldPosition(const Vector3& pos);
		void									setWorldScale(const Vector3& scale);
		void									setWorldEulerRotation(const Vector3& rotation);
		void									setWorldQuaternion(const Vector3& rotation);

		void									updateNode(bool childUpdate = true);

	private:
		std::vector<EComponent*>						m_components;
		std::map<std::string, EComponent*>				m_componentsMap; //组件类型名，对应组件指针
		std::map<EComponent*, size_t>					m_componentsIndexMap;
		std::vector<EGameObject*>						m_childGameObjects;
		std::map<std::string, EGameObject*>				m_childGameObjectMap;
		std::map<EGameObject*, size_t>					m_childGameObjectIdxMap;
		EGameObjectInfo*								m_info = nullptr;
		SceneNode*										m_pSceneNode = nullptr;
		EGameObject*									m_pParentGObject = nullptr;


		Vector3											m_WorldPosition;
		Vector3											m_WorldScale;
		Vector3											m_WorldRotation;
		Quaternion										m_WorldQuaternion;
	};
}
