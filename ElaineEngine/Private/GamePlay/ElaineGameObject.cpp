#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineGameObject.h"
#include "GamePlay/ElaineComponent.h"
#include "GamePlay/ElaineGameObjectMgr.h"
#include "GamePlay/ElaineTransformComponent.h"
#include "GamePlay/ElaineMeshComponent.h"
#include "ElaineDataStream.h"
#include "ElaineGameObjectInfoMgr.h"
#include "ElaineWorld.h"
#include "math/ElaineDynamicBVH.h"

namespace Elaine
{
	GameObjectInfo::GameObjectInfo()
	{

	}

	GameObjectInfo::GameObjectInfo(ResourceManager* pManager, const std::string& path)
		: ResourceBase(pManager, path)
	{

	}

	GameObjectInfo::~GameObjectInfo()
	{

	}

	bool GameObjectInfo::LoadImpl()
	{
		DataStream JsonFileStream(Root::instance()->GetResourcePath() + mResourceName, DataStream::Out);
		JsonFileStream.ReadAll();
		JsonCpp JsonData(JsonFileStream.GetDataStream());
		ImportData(JsonData);

		return true;
	}

	void GameObjectInfo::UnloadImpl()
	{
		ExportToFile();
	}

	void GameObjectInfo::SaveResourceImpl()
	{
	}

	void GameObjectInfo::ResourceArrivedImpl()
	{
	}

	void GameObjectInfo::ExportToFile()
	{
		JsonCpp JsonData;
		ExportData(JsonData);

		DataStream JsonFileStream(Root::instance()->GetResourcePath() + mResourceName, DataStream::Out);
		std::string JsonStr = JsonData.dump();
		JsonFileStream.Write(JsonStr.data(), JsonStr.size());
	}

	void GameObjectInfo::ImportData(const JsonCpp& jsonNode)
	{
		if (jsonNode.is_null())
			return;

		if (jsonNode.contains("Name"))
			mName = jsonNode["Name"].get<std::string>();

		if (jsonNode.contains("GUID"))
			mGUID = jsonNode["GUID"].get<std::string>();

		// Components
		if (jsonNode.contains("ComponentArray"))
		{
			for (const auto& CompJson : jsonNode["ComponentArray"])
			{
				if (CompJson.contains("Type"))
				{
					Name ComType = CompJson["Type"].get<std::string>().c_str();
					ComponentInfo* NewComInfo = ComponentFactoryManager::instance()->CreateComponentInfo(ComType);
					if (NewComInfo != nullptr)
					{
						NewComInfo->ImportData(CompJson);
						mComponentInfos.push_back(NewComInfo);
					}
				}
			}
		}

		// Children GameObjects
		if (jsonNode.contains("GameObjectArray"))
		{
			for (const auto& childJson : jsonNode["GameObjectArray"])
			{
				GameObjectInfoPtr NewGameObjectInfo = GameObjectInfoMgr::instance()->GetResource<GameObjectInfo>("");

				NewGameObjectInfo->ImportData(childJson);
				mChildren.push_back(NewGameObjectInfo);
			}
		}
	}

	void GameObjectInfo::ExportData(JsonCpp& jsonNode)
	{
		jsonNode["Name"] = mName;
		jsonNode["GUID"] = mGUID;

		// ComponentArray
		jsonNode["ComponentArray"] = JsonCpp::array();
		for (auto* comp : mComponentInfos)
		{
			JsonCpp compJson;
			comp->ExportData(compJson);
			jsonNode["ComponentArray"].push_back(compJson);
		}

		// GameObjectArray
		jsonNode["GameObjectArray"] = JsonCpp::array();
		for (auto& child : mChildren)
		{
			JsonCpp childJson;
			child->ExportData(childJson);
			jsonNode["GameObjectArray"].push_back(childJson);
		}
	}



	GameObject::GameObject(World* InWorld)
		: mWorld(InWorld)
	{

	}

	GameObject::GameObject(const std::string& InName)
	{

	}

	GameObject::~GameObject()
	{
		Destroy();
	}

	void GameObject::SetName(const std::string& InName)
	{
		mName = InName;
	}

	void GameObject::Initialize(GameObjectInfoPtr InInfo)
	{
		if (mbInitialized)
			return;

		if (InInfo == nullptr)
		{
			Initialize();
			return;
		}

		for (auto ComInfo : InInfo->mComponentInfos)
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
			GameObject* childGo = mWorld->GetGameObjectMgr()->CreateGameObjectByInfo(GoInfo);
			AddChildGameObject(childGo);
		}

		mbInitialized = true;
	}

	void GameObject::Initialize()
	{
		if (mbInitialized)
			return;

		mTransformCom = static_cast<TransformComponent*>(ComponentFactoryManager::instance()->CreateComponent(Name("TransformComponent"), this));
		AddComponent(mTransformCom);

		SetName(mNameGenerator());

		mbInitialized = true;
	}

	void GameObject::save()
	{
		mDescription->ExportToFile();
	}

	Component* GameObject::AddComponent(const Name& InType)
	{
		Component* NewComponent = ComponentFactoryManager::instance()->CreateComponent(InType, this);
		if (NewComponent != nullptr)
		{
			NewComponent->Initialize(nullptr);
		}
		mComponents.emplace(InType, NewComponent);

		return NewComponent;
	}

	SceneManager* GameObject::GetSceneManager() const
	{
		return mWorld->GetSceneManager();
	}

	Component* GameObject::GetComponentByName(const Name& name)
	{
		auto it = mComponents.find(name);
		if (it != mComponents.end())
			return (*it).second;
		return nullptr;
	}

	void GameObject::AddChildGameObject(GameObject* InChild)
	{
		if (InChild == nullptr)
			return;

		mChildren.push_back(InChild);
		mChildrenMap[mDescription->mGUID] = InChild;
	}

	void GameObject::AddComponent(Component* InCom)
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

		//InCom->OnRegisterWorld(mWorld);
	}

	GameObject* GameObject::CreateChildGameObject()
	{
		GameObject* newGo = mWorld->GetGameObjectMgr()->CreateGameObject();
		AddChildGameObject(newGo);
		return newGo;
	}

	void GameObject::AddWorldOffset(const Vector3& InDelta, bool InRecursive)
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

	void GameObject::Destroy()
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

	void GameObject::RemoveComponent(Component* InComponent)
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

		Component* removeCom = itIdx->first;
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

	void GameObject::RemoveChildGameObject(GameObject* InObject)
	{
		if (InObject == nullptr)
			return;

		auto Iter = mChildrenMap.find(InObject->GetName());
		if (Iter != mChildrenMap.end())
		{
			mChildrenMap.erase(Iter);
		}

		mWorld->GetGameObjectMgr()->DestroyGameObject(InObject);
	}

	const Vector3& GameObject::GetWorldPosition() const
	{
		return mTransformCom->GetWorldPosition();
	}

	const Vector3& GameObject::GetWorldScale() const
	{
		return mTransformCom->GetWorldScale();
	}

	const Quaternion& GameObject::GetWorldRotation() const
	{
		return mTransformCom->GetWorldRotation();
	}

	const Matrix4x4& GameObject::GetWorldMatrix() const
	{
		return mTransformCom->GetWorldMatrix();
	}

	const Vector3& GameObject::GetPosition() const
	{
		return mTransformCom->GetPosition();
	}

	const Vector3& GameObject::GetScale() const
	{
		return mTransformCom->GetScale();
	}

	const Quaternion& GameObject::GetRotation() const
	{
		return mTransformCom->GetRotation();
	}

	void GameObject::SetPosition(const Vector3& InPosition)
	{
		mTransformCom->SetPosition(InPosition);
        if (mWorld && mWorld->GetSceneBVH()) mWorld->GetSceneBVH()->UpdateObject(this);
	}

	void GameObject::SetScale(const Vector3& InScale)
	{
		mTransformCom->SetScale(InScale);
        if (mWorld && mWorld->GetSceneBVH()) mWorld->GetSceneBVH()->UpdateObject(this);
	}

	void GameObject::SetQuaternion(const Quaternion& InRotation)
	{
		mTransformCom->SetRotation(InRotation);
        if (mWorld && mWorld->GetSceneBVH()) mWorld->GetSceneBVH()->UpdateObject(this);
	}

	void GameObject::UpdateNode(bool childUpdate /*= true*/, bool notifyParent /*= true*/)
	{

	}

	void GameObject::OnRegisterWorld(World* InWorld)
	{
		mWorld = InWorld;
		for (auto&& Com : mComponents)
		{
			Com.second->OnRegisterWorld(InWorld);
		}

        if (mWorld && mWorld->GetSceneBVH())
            mWorld->GetSceneBVH()->InsertObject(this);
	}

	void GameObject::OnUnregisterWorld()
	{
        if (mWorld && mWorld->GetSceneBVH() && mBVHNodeID != -1)
            mWorld->GetSceneBVH()->RemoveObject(this);

		for (auto&& Com : mComponents)
		{
			Com.second->OnUnregisterWorld();
		}
        mWorld = nullptr;
	}

	AxisAlignedBox GameObject::GetBoundingBox() const
	{
		// Accumulate AABB from visual/physical components
		AxisAlignedBox Box;
		Box.setNull();

		// Check logic for finding mesh component
		for (auto& Pair : mComponents)
		{
			//if (MeshComponent* MeshComp = dynamic_cast<MeshComponent*>(Pair.second))
			//{
			//	Box.merge(MeshComp->GetBoundingBox()); 
			//}
		}

		if (Box.isNull())
		{
			// Fallback if no specific component provides bounds: create small bound around position
			Vector3 Pos = GetWorldPosition();
			//Box.setExtents(Pos - Vector3(0.5f, 0.5f, 0.5f), Pos + Vector3(0.5f, 0.5f, 0.5f));
		}

		return Box;
	}
}