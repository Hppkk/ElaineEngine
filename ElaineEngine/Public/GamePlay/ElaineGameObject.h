#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineResourceBase.h"
#include "GamePlay/ElaineComponent.h"


/*----------------------------------------------
*Engine Component Architecture: one gameobject each type of component can only hold one.
--------------------------------------------*/

namespace Elaine
{
	class GameObjectInfo;
	using GameObjectInfoPtr = ResourcePtr<GameObjectInfo>;


	class ElaineEngineExport GameObjectInfo :public ResourceBase
	{
		friend class GameObject;
	public:
		GameObjectInfo();
		GameObjectInfo(ResourceManager* InManager, const std::string& InPath);
		~GameObjectInfo();
		virtual bool	LoadImpl() override;
		virtual	void	UnloadImpl() override;
		virtual void	SaveResourceImpl() override;
		virtual void	ResourceArrivedImpl() override;
		void			ImportData(const JsonCpp& jsonNode);
		void			ExportData(JsonCpp& jsonNode);
		const std::string& GetName() const { return mName; }
		void SetName(const std::string& InName) { mName = InName; }
		void			ExportToFile();
	private:
		std::string mGUID;
		std::string mName;
		std::set<GameObject*> mInstances;
		std::vector<GameObjectInfoPtr> mChildren;
		std::vector<ComponentInfo*> mComponentInfos;
	};
	
	class GameObjectNameGenerator
	{
	public:
		GameObjectNameGenerator() = default;

		std::string operator()()
		{
			return getNewName();
		}

		std::string getNewName()
		{
			if (mNextIndex == 0)
			{
				mNextIndex++;
				return "GameObject";
			}
			return std::format("GameObject({})", mNextIndex);
		}
	private:
		size_t mNextIndex = 0;
	};

	class Component;
	class World;
	class TransformComponent;
	class SceneManager;

	class ElaineEngineExport GameObject
	{
		friend class GameObjectMgr;
	public:
		GameObject(World* InWorld);
		GameObject(const std::string& InName);
		~GameObject();
		const std::string&					GetName() const { return mName; }
		void								SetName(const std::string& InName);
		void								Initialize(GameObjectInfoPtr info);
		void								Initialize();
		Component*							GetComponentByName(const Name& name);
#ifdef _HAS_EDITOR_
		std::vector<Component*>&			GetEditorComponents() { return m_components; }
#endif
		std::map<Name, Component*>&			GetComponents() { return mComponents; }
		void								AddChildGameObject(GameObject* InChild);
		void								AddComponent(Component* InCom);
		GameObject*							CreateChildGameObject();
		void								AddWorldOffset(const Vector3& InDelta, bool InRecursive = true);
		void								Destroy();
		void								RemoveComponent(Component* InComponent);
		void								RemoveChildGameObject(GameObject* InObject);
		void								save();
		Component*							AddComponent(const Name& InType);
		SceneManager*						GetSceneManager() const;

		template<typename ComponentType>
		ComponentType* AddComponentType(const Name& InType)
		{
			return static_cast<ComponentType*>(AddComponent(InType));
		}
		template<typename ComponentType>
		ComponentType* GetComponent()
		{
			for (auto& com : mComponents)
			{
				if(com.second->mType == ComponentType::mType)
					return static_cast<ComponentType*>(com.second);
			}
			return nullptr;
		}

		GameObject*							GetParent() { return mParent; }
		const Vector3&						GetWorldPosition() const;
		const Vector3&						GetWorldScale() const;
		const Quaternion&					GetWorldRotation() const;
		const Matrix4x4&					GetWorldMatrix() const;
		const Vector3&						GetPosition() const;
		const Vector3&						GetScale() const;
		const Quaternion&					GetRotation() const;
		void								SetPosition(const Vector3& pos);
		void								SetScale(const Vector3& scale);
		void								SetQuaternion(const Quaternion& rotation);
		void								UpdateNode(bool childUpdate = true, bool notifyParent = true);
		void								OnRegisterWorld(World* InWorld);
		void								OnUnregisterWorld();
	private:
#ifdef _HAS_EDITOR_
		std::vector<Component*>				m_components;
		std::map<Component*, size_t>		m_componentsIndexMap;
#endif
		std::map<Name, Component*>			mComponents; //组件类型名，对应组件指针
		std::vector<GameObject*>			mChildren;
		std::map<std::string, GameObject*>	mChildrenMap;
		GameObjectInfo*						mDescription = nullptr;
		GameObject*							mParent = nullptr;
		World*								mWorld = nullptr;
		TransformComponent*					mTransformCom = nullptr;
		std::string							mName;
		GameObjectNameGenerator				mNameGenerator;
		bool								mbInitialized = false;
		friend class World;
	};
}
