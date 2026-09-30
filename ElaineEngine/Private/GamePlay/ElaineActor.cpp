#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineActor.h"
#include "GamePlay/ElaineComponent.h"
#include "GamePlay/ElaineActorManager.h"
#include "GamePlay/ElaineTransformComponent.h"
#include "GamePlay/ElaineMeshComponent.h"
#include "ElaineDataStream.h"
#include "ElaineActorInfoMgr.h"
#include "ElaineWorld.h"
#include "math/ElaineDynamicBVH.h"

namespace Elaine
{
	ActorInfo::ActorInfo()
	{

	}

	ActorInfo::ActorInfo(ResourceManager* pManager, const std::string& path)
		: ResourceBase(pManager, path)
	{

	}

	ActorInfo::~ActorInfo()
	{

	}

	bool ActorInfo::LoadImpl()
	{
		DataStream JsonFileStream(Root::instance()->GetResourcePath() + mResourceName, DataStream::In);
		JsonFileStream.ReadAll();
		JsonCpp JsonData(JsonFileStream.GetDataStream());
		ImportData(JsonData);

		return true;
	}

	void ActorInfo::UnloadImpl()
	{
		ExportToFile();
	}

	void ActorInfo::SaveResourceImpl()
	{
	}

	void ActorInfo::ResourceArrivedImpl()
	{
	}

	void ActorInfo::ExportToFile()
	{
		JsonCpp JsonData;
		ExportData(JsonData);

		DataStream JsonFileStream(Root::instance()->GetResourcePath() + mResourceName, DataStream::Out);
		std::string JsonStr = JsonData.dump();
		JsonFileStream.Write(JsonStr.data(), JsonStr.size());
	}

	void ActorInfo::ImportData(const JsonCpp& jsonNode)
	{
		if (jsonNode.is_null())
			return;

		if (jsonNode.contains("Name"))
			mName = jsonNode["Name"].get<std::string>();

		if (jsonNode.contains("GUID"))
			mGUID = jsonNode["GUID"].get<std::string>();

		// Components
		if (jsonNode.contains("Components"))
		{
			for (const auto& CompJson : jsonNode["Components"])
			{
				if (CompJson.contains("Type"))
				{
					Name ComType = CompJson["Type"].get<std::string>().c_str();
					ActorComponentInfo* NewComInfo = ComponentFactoryManager::instance()->CreateActorComponentInfo(ComType);
					if (NewComInfo != nullptr)
					{
						NewComInfo->ImportData(CompJson);
						mActorComponentInfos.push_back(NewComInfo);
					}
				}
			}
		}

		// Children Actors
		if (jsonNode.contains("Children"))
		{
			for (const auto& childJson : jsonNode["Children"])
			{
				ActorInfoPtr NewActorInfo = ActorInfoMgr::instance()->GetResource<ActorInfo>("");

				NewActorInfo->ImportData(childJson);
				mChildren.push_back(NewActorInfo);
			}
		}
	}

	void ActorInfo::ExportData(JsonCpp& jsonNode)
	{
		jsonNode["Name"] = mName;
		jsonNode["GUID"] = mGUID;

		// Components
		jsonNode["Components"] = JsonCpp::array();
		for (auto* comp : mActorComponentInfos)
		{
			JsonCpp compJson;
			comp->ExportData(compJson);
			jsonNode["Components"].push_back(compJson);
		}

		// Children
		jsonNode["Children"] = JsonCpp::array();
		for (auto& child : mChildren)
		{
			JsonCpp childJson;
			child->ExportData(childJson);
			jsonNode["Children"].push_back(childJson);
		}
	}



	Actor::Actor(World* InWorld)
		: mWorld(InWorld)
	{

	}

	Actor::Actor(const std::string& InName)
	{

	}

	Actor::~Actor()
	{
		Destroy();
	}

	void Actor::SetName(const std::string& InName)
	{
		mName = InName;
	}

	void Actor::Initialize(ActorInfoPtr InInfo)
	{
		if (mbInitialized)
			return;

		if (InInfo == nullptr)
		{
			Initialize();
			return;
		}

		for (auto ComInfo : InInfo->mActorComponentInfos)
		{
			auto ComFactroy = ComponentFactoryManager::instance()->GetComponentFactory(ComInfo->mType);
			if (ComFactroy)
			{
				auto NewComponent = ComFactroy->CreateComponent(this);
				NewComponent->Initialize(ComInfo);
				AddComponent(NewComponent);
			}
		}

		for (auto GoInfo : InInfo->mChildren)
		{
			Actor* childGo = mWorld->GetActorManager()->CreateActorByInfo(GoInfo);
			AddChildActor(childGo);
		}
		if (mTransformCom == nullptr)
		{
			mTransformCom = static_cast<TransformComponent*>(ComponentFactoryManager::instance()->CreateComponent(Name("TransformComponent"), this));
			AddComponent(mTransformCom);
		}
		SetName(InInfo->GetName());

		mbInitialized = true;
	}

	void Actor::Initialize()
	{
		if (mbInitialized)
			return;

		mTransformCom = static_cast<TransformComponent*>(ComponentFactoryManager::instance()->CreateComponent(Name("TransformComponent"), this));
		AddComponent(mTransformCom);

		SetName(mNameGenerator());

		mbInitialized = true;
	}

	void Actor::save()
	{
		mDescription->ExportToFile();
	}

	ActorComponent* Actor::AddComponent(const Name& InType)
	{
		ActorComponent* NewComponent = ComponentFactoryManager::instance()->CreateComponent(InType, this);
		if (NewComponent != nullptr)
		{
			NewComponent->Initialize(nullptr);
		}
		mComponents.emplace(InType, NewComponent);

		return NewComponent;
	}

	SceneManager* Actor::GetSceneManager() const
	{
		return mWorld->GetSceneManager();
	}

	ActorComponent* Actor::GetComponentByName(const Name& name)
	{
		auto it = mComponents.find(name);
		if (it != mComponents.end())
			return (*it).second;
		return nullptr;
	}

	void Actor::AddChildActor(Actor* InChild)
	{
		if (InChild == nullptr)
			return;

		mChildren.push_back(InChild);
		InChild->mParent = this;
		mChildrenMap[InChild->GetName()] = InChild;
	}

	void Actor::AddComponent(ActorComponent* InCom)
	{
		if (InCom == nullptr)
			return;

		if (mComponents.find(InCom->GetType()) != mComponents.end())
			return;

#ifdef _HAS_EDITOR_
		auto iter = m_componentsIndexMap.find(InCom);
		if (iter != m_componentsIndexMap.end())
			return;
		
		m_componentsIndexMap[InCom] = m_components.size();
		m_components.push_back(InCom);
#endif
		mComponents[InCom->GetType()] = InCom;
		if (auto* Transform = dynamic_cast<TransformComponent*>(InCom))
			mTransformCom = Transform;

		//InCom->OnRegisterWorld(mWorld);
	}

	Actor* Actor::CreateChildActor()
	{
		Actor* newGo = mWorld->GetActorManager()->CreateActor();
		AddChildActor(newGo);
		return newGo;
	}

	void Actor::AddWorldOffset(const Vector3& InDelta, bool InRecursive)
	{
		if (mTransformCom)
		{
			//mTransformCom->AddWorldOffset(InDelta);
            if (mWorld && mWorld->GetSceneBVH())
                mWorld->GetSceneBVH()->UpdateObject(this);
		}

		if (InRecursive)
		{
			for (auto* child : mChildren)
			{
				if (child)
					child->AddWorldOffset(InDelta, true);
			}
		}
	}

	void Actor::Destroy()
	{
		for (auto com : mComponents)
		{
			if (!com.second)continue;

			auto factory = ComponentFactoryManager::instance()->GetComponentFactory(com.second->GetType());
			if (factory)
			{
				factory->DestoryComponent(com.second);
			}
		}

		mComponents.clear();
		for (auto go : mChildren)
		{
			if (!go)continue;

			go->Destroy();
		}
		mChildren.clear();
	}

	void Actor::RemoveComponent(ActorComponent* InComponent)
	{
		if (InComponent == nullptr)
			return;

		auto it = mComponents.find(InComponent->GetType());
		if (it == mComponents.end())
			return;

#ifdef _HAS_EDITOR_
		auto itIdx = m_componentsIndexMap.find(InComponent);
		if (itIdx == m_componentsIndexMap.end())
			return;

		ActorComponent* removeCom = itIdx->first;
		size_t		idx = itIdx->second;
		m_componentsIndexMap.erase(itIdx);
		auto iter = m_components.begin() + idx;
		m_components.erase(iter);
#endif
		
		auto factory = ComponentFactoryManager::instance()->GetComponentFactory(InComponent->GetType());
		if (factory == nullptr)
			return;

		factory->DestoryComponent(InComponent);
	}

	void Actor::RemoveChildActor(Actor* InObject)
	{
		if (InObject == nullptr)
			return;

		auto Iter = mChildrenMap.find(InObject->GetName());
		if (Iter != mChildrenMap.end())
		{
			mChildrenMap.erase(Iter);
		}

		mWorld->GetActorManager()->DestroyActor(InObject);
	}

	const Vector3& Actor::GetWorldPosition() const
	{
		return mTransformCom->GetWorldPosition();
	}

	const Vector3& Actor::GetWorldScale() const
	{
		return mTransformCom->GetWorldScale();
	}

	const Quaternion& Actor::GetWorldRotation() const
	{
		return mTransformCom->GetWorldRotation();
	}

	const Matrix4x4& Actor::GetWorldMatrix() const
	{
		return mTransformCom->GetWorldMatrix();
	}

	const Vector3& Actor::GetPosition() const
	{
		return mTransformCom->GetPosition();
	}

	const Vector3& Actor::GetScale() const
	{
		return mTransformCom->GetScale();
	}

	const Quaternion& Actor::GetRotation() const
	{
		return mTransformCom->GetRotation();
	}

	void Actor::SetPosition(const Vector3& InPosition)
	{
		mTransformCom->SetPosition(InPosition);
        if (mWorld && mWorld->GetSceneBVH()) mWorld->GetSceneBVH()->UpdateObject(this);
	}

	void Actor::SetScale(const Vector3& InScale)
	{
		mTransformCom->SetScale(InScale);
        if (mWorld && mWorld->GetSceneBVH()) mWorld->GetSceneBVH()->UpdateObject(this);
	}

	void Actor::SetQuaternion(const Quaternion& InRotation)
	{
		mTransformCom->SetRotation(InRotation);
        if (mWorld && mWorld->GetSceneBVH()) mWorld->GetSceneBVH()->UpdateObject(this);
	}

	void Actor::UpdateNode(bool childUpdate /*= true*/, bool notifyParent /*= true*/)
	{

	}

	void Actor::OnRegisterWorld(World* InWorld)
	{
		mWorld = InWorld;
		for (auto&& Com : mComponents)
		{
			Com.second->OnRegisterWorld(InWorld);
		}

        if (mWorld && mWorld->GetSceneBVH())
            mWorld->GetSceneBVH()->InsertObject(this);
	}

	void Actor::OnUnregisterWorld()
	{
        if (mWorld && mWorld->GetSceneBVH() && mBVHNodeID != -1)
            mWorld->GetSceneBVH()->RemoveObject(this);

		for (auto&& Com : mComponents)
		{
			Com.second->OnUnregisterWorld();
		}
        mWorld = nullptr;
	}

	AxisAlignedBox Actor::GetBoundingBox() const
	{
		// Accumulate AABB from visual/physical components
		AxisAlignedBox Box;
		Box.setNull();

		// Check logic for finding mesh ActorComponent
		for (auto& Pair : mComponents)
		{
			//if (MeshComponent* MeshComp = dynamic_cast<MeshComponent*>(Pair.second))
			//{
			//	Box.merge(MeshComp->GetBoundingBox()); 
			//}
		}

		if (Box.isNull())
		{
			// Fallback if no specific ActorComponent provides bounds: create small bound around position
			Vector3 Pos = GetWorldPosition();
			//Box.setExtents(Pos - Vector3(0.5f, 0.5f, 0.5f), Pos + Vector3(0.5f, 0.5f, 0.5f));
		}

		return Box;
	}
}
