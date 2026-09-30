#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineResourceBase.h"
#include "ElaineReflectionDefines.h"
#include "GamePlay/ElaineComponent.h"
#include "ElaineMatrix4.h"
#include "math/ElaineISpatialObject.h"
#include "ElaineActor.generated.h"


/*----------------------------------------------
*Engine ActorComponent Architecture: one Actor each type of ActorComponent can only hold one.
--------------------------------------------*/

namespace Elaine
{
	class ActorInfo;
	using ActorInfoPtr = ResourcePtr<ActorInfo>;


	class ElaineEngineExport ActorInfo :public ResourceBase
	{
		friend class Actor;
	public:
		ActorInfo();
		ActorInfo(ResourceManager* InManager, const std::string& InPath);
		~ActorInfo();
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
		std::set<Actor*> mInstances;
		std::vector<ActorInfoPtr> mChildren;
		std::vector<ActorComponentInfo*> mActorComponentInfos;
	};
	
	class ActorNameGenerator
	{
	public:
		ActorNameGenerator() = default;

		std::string operator()()
		{
			return getNewName();
		}

		std::string getNewName()
		{
			if (mNextIndex == 0)
			{
				mNextIndex++;
				return "Actor";
			}
			return std::format("Actor({})", mNextIndex);
		}
	private:
		size_t mNextIndex = 0;
	};

	class ActorComponent;
	class World;
	class TransformComponent;
	class SceneManager;

	ECLASS(DisplayName = "Actor")
	class ElaineEngineExport Actor : public ISpatialObject
	{
		GENERATED_BODY()
		friend class ActorManager;
	public:
		Actor(World* InWorld);
		Actor(const std::string& InName);
		~Actor();
		const std::string&				GetName() const { return mName; }
		EFUNCTION(DisplayName="Set Name", Category="Actor")
		void							SetName(const std::string& InName);
		void							Initialize(ActorInfoPtr info);
		void							Initialize();
		ActorComponent*						GetComponentByName(const Name& name);
#ifdef _HAS_EDITOR_
		std::vector<ActorComponent*>&		GetEditorComponents() { return m_components; }
#endif
		std::map<Name, ActorComponent*>&		GetComponents() { return mComponents; }
		void							AddChildActor(Actor* InChild);
		void							AddComponent(ActorComponent* InCom);
		Actor*						CreateChildActor();
		EFUNCTION(DisplayName="Add World Offset", Category="Transform")
		void							AddWorldOffset(const Vector3& InDelta, bool InRecursive = true);
		EFUNCTION(Category="Lifecycle")
		void							Destroy();
		void							RemoveComponent(ActorComponent* InComponent);
		void							RemoveChildActor(Actor* InObject);
		void							save();
		ActorComponent*						AddComponent(const Name& InType);
		SceneManager*					GetSceneManager() const;

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
				if (auto* TypedComponent = dynamic_cast<ComponentType*>(com.second))
					return TypedComponent;
			}
			return nullptr;
		}

		Actor*						GetParent() { return mParent; }
		const Vector3&					GetWorldPosition() const;
		const Vector3&					GetWorldScale() const;
		const Quaternion&				GetWorldRotation() const;
		const Matrix4x4&				GetWorldMatrix() const;
		const Vector3&					GetPosition() const;
		const Vector3&					GetScale() const;
		const Quaternion&				GetRotation() const;
		EFUNCTION(DisplayName="Set Position", Category="Transform")
		void							SetPosition(const Vector3& pos);
		EFUNCTION(DisplayName="Set Scale", Category="Transform")
		void							SetScale(const Vector3& scale);
		EFUNCTION(DisplayName="Set Rotation", Category="Transform")
		void							SetQuaternion(const Quaternion& rotation);
		void							UpdateNode(bool childUpdate = true, bool notifyParent = true);
		void							OnRegisterWorld(World* InWorld);
		void							OnUnregisterWorld();

		// ISpatialObject interface
		virtual AxisAlignedBox			GetBoundingBox() const override;
		virtual void*					GetUserData() const override { return const_cast<Actor*>(this); }
		virtual uint32_t				GetUserType() const override { return 1; } // 1 = Actor
		virtual void					SetBVHNodeID(int32_t ID) override { mBVHNodeID = ID; }
		virtual int32_t					GetBVHNodeID() const override { return mBVHNodeID; }
		
	private:
#ifdef _HAS_EDITOR_
		std::vector<ActorComponent*>			m_components;
		std::map<ActorComponent*, size_t>	m_componentsIndexMap;
#endif
		std::map<Name, ActorComponent*>		mComponents;
		std::vector<Actor*>		mChildren;
		std::map<std::string, Actor*>	mChildrenMap;
		ActorInfo*					mDescription = nullptr;
		Actor*						mParent = nullptr;
		World*							mWorld = nullptr;
		TransformComponent*				mTransformCom = nullptr;
		EPROPERTY(DisplayName="Name", Category="Actor", Tooltip="The name of this game object")
		std::string						mName;
		ActorNameGenerator			mNameGenerator;
		bool							mbInitialized = false;

		// ISpatialObject state
		int32_t							mBVHNodeID = -1;

		friend class World;
	};
}
