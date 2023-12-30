#pragma once
#include "ElaineCorePrerequirements.h"
#include "resource/ElaineResourceBase.h"
#include "component/ElaineComponent.h"


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
		EGameObjectInfo(ResourceManager* pManager, const std::string& path);
		~EGameObjectInfo();
		virtual void			loadImpl() override;
		virtual	void			unloadImpl() override;
		void					importData(cJSON* jsonNode);
		void					exportData(cJSON* jsonNode);
		std::string&			getGameObjectName() { return m_sName; }
		void					setGameObjectName(const std::string& _name) { m_sName = _name; }
		void					exportToFile();


	private:
		std::string						m_sFilePath;
		std::set<EGameObject*>			m_InstGameObjectSet;
		std::string						m_sGUID;
		std::vector<EComponentInfo*>	m_Components;
		std::vector<EGameObjectInfo*>	m_childGameObjectInfos;
		std::string						m_sParentGUID;
		std::string						m_sName;
	};

	
	class GameObjectNameGenerater
	{
	public:
		GameObjectNameGenerater()
		{

		}

		std::string getNewName()
		{
			if (m_nNextIdx == 0)
			{
				m_nNextIdx++;
				return "GameObject";
			}
			return "GameObject (" + std::to_string(m_nNextIdx) + ")";
		}
	private:
		size_t				m_nNextIdx = 0;
	};

	class EComponent;
	class SceneNode;
	class ElaineCoreExport EGameObject
	{
		friend class GameObjectMgr;
	public:
		EGameObject();
		EGameObject(const std::string& name);
		~EGameObject();
		void									init(EGameObjectInfo* info);
		EComponent*								GetComponentByName(const std::string& name);
#ifdef _HAS_EDITOR
		std::vector<EComponent*>&				GetEditorComponents() { return m_components; }
#endif
		std::map<std::string, EComponent*>& GetComponents() { return m_componentsMap; }
		void									AddChildGameObject(EGameObject* obj);
		void									AddComponent(EComponent* com);
		EGameObject*							CreateChildGameObject();
		void									destroy();
		void									destoryImpl();
		void									removeComponent(EComponent* rhs);
		void									removeChildGameObject(EGameObject* rhs);
		void									save();
		template<typename ComponentType>
		ComponentType* GetComponent()
		{
			for (auto& com : m_componentsMap)
			{
				if(com.second->m_sType == ComponentType::m_sType)
					return static_cast<ComponentType*>(com.second);
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
		void									setWorldQuaternion(const Quaternion& rotation);

		void									updateNode(bool childUpdate = true, bool notifyParent = true);

	private:
#ifdef _HAS_EDITOR_
		std::vector<EComponent*>						m_components;
		std::map<EComponent*, size_t>					m_componentsIndexMap;
#endif
		std::map<std::string, EComponent*>				m_componentsMap; //组件类型名，对应组件指针
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
		GameObjectNameGenerater							m_NameGener;
	};
}
