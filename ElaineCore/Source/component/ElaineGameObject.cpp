#include "ElainePrecompiledHeader.h"
#include "component/ElaineGameObject.h"
#include "component/ElaineComponent.h"
#include "component/ElaineGameObjectMgr.h"
#include "component/ElaineTransformComponent.h"

namespace Elaine
{
	EGameObjectInfo::EGameObjectInfo()
	{

	}

	EGameObjectInfo::EGameObjectInfo(ResourceManager* pManager, const std::string& path)
		: ResourceBase(pManager, path)
	{

	}

	EGameObjectInfo::~EGameObjectInfo()
	{
		for (auto& comInfo : m_Components)
		{
			SAFE_DELETE(comInfo);
		}
	}

	void EGameObjectInfo::loadImpl()
	{
		std::ifstream file;
		file.open(m_sFilePath.c_str());
		std::stringstream buffer;
		buffer << file.rdbuf();
		cJSON* jsonNode = cJSON_Parse(buffer.str().c_str());
		importData(jsonNode);
		file.close();
	}

	void EGameObjectInfo::unloadImpl()
	{
		exportToFile();
	}

	void EGameObjectInfo::exportToFile()
	{
		cJSON* jsonNode = cJSON_CreateObject();
		exportData(jsonNode);
		char* charValue = cJSON_Print(jsonNode);
		std::ofstream outFile;
		outFile.open(m_sFilePath.c_str());
		outFile << charValue;
		outFile.close();
		delete[] charValue;
	}

	void EGameObjectInfo::importData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;
		cJSON* name_ = cJSON_GetObjectItem(jsonNode, "Name");
		if (name_)
			m_sName = name_->valuestring;
		cJSON* guid_ = cJSON_GetObjectItem(jsonNode, "GUID");
		if (guid_)
			m_sGUID = guid_->valuestring;
		cJSON* parentGUID_ = cJSON_GetObjectItem(jsonNode, "ParentGUID");
		if (parentGUID_)
			m_sGUID = parentGUID_->valuestring;

		cJSON* pComponentArray = cJSON_GetObjectItem(jsonNode, "ComponentArray");
		if (pComponentArray)
		{
			for (auto pChild = pComponentArray->child; pChild != nullptr; pChild = pChild->next)
			{
				EComponentInfo* info = new EComponentInfo();
				info->importData(pChild);
				m_Components.push_back(info);
			}
		}

		cJSON* pGameObjectArray = cJSON_GetObjectItem(jsonNode, "GameObjectArray");
		if (pGameObjectArray)
		{
			for (auto pChild = pGameObjectArray->child; pChild != nullptr; pChild = pChild->next)
			{
				EGameObjectInfo* info = static_cast<EGameObjectInfo*>(GameObjectInfoMgr::instance()->createResource("").get());
				info->importData(pChild);
				m_childGameObjectInfos.push_back(info);
			}
		}
	}

	void EGameObjectInfo::exportData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;

		cJSON_AddItemToObject(jsonNode, "Name", cJSON_CreateString(m_sName.c_str()));
		cJSON_AddItemToObject(jsonNode, "GUID", cJSON_CreateString(m_sGUID.c_str()));
		cJSON_AddItemToObject(jsonNode, "ParentGUID", cJSON_CreateString(m_sParentGUID.c_str()));

		cJSON* pComponentArray =  cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "ComponentArray", pComponentArray);
		for (size_t i = 0; i < m_Components.size(); ++i)
		{
			cJSON* pChild = cJSON_CreateObject();
			cJSON_AddItemToArray(pComponentArray, pChild);
			m_Components[i]->exportData(pChild);
		}

		cJSON* pGameObjectArray = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "GameObjectArray", pGameObjectArray);
		for (size_t i = 0; i < m_childGameObjectInfos.size(); ++i)
		{
			cJSON* pChild = cJSON_CreateObject();
			cJSON_AddItemToArray(pGameObjectArray, pChild);
			m_Components[i]->exportData(pChild);
		}

	}



	EGameObject::EGameObject()
	{

	}

	EGameObject::EGameObject(const std::string& name)
	{

	}

	EGameObject::~EGameObject()
	{
		destroy();
	}

	void EGameObject::init(EGameObjectInfo* info)
	{

		for (auto cominfo : info->m_Components)
		{
			auto factroy = ComponentFactoryManager::instance()->getFactoryByComType(cominfo->m_sType);
			if (factroy)
			{
				if (cominfo->m_sType == "TransformComponent")
				{
					auto tranInfo = static_cast<TransformComponentInfo*>(cominfo);
					setWorldPosition(tranInfo->m_pTransform->m_position);
					setWorldQuaternion(tranInfo->m_pTransform->m_rotation);
					setWorldScale(tranInfo->m_pTransform->m_scale);
				}

				auto pComponent = factroy->createComponent();
				pComponent->init(cominfo);
				AddComponent(pComponent);
			}
		}

		for (auto goInfo : info->m_childGameObjectInfos)
		{
			EGameObject* childGo = GameObjectMgr::instance()->createGameObjectByInfo(info);
			AddChildGameObject(childGo);
		}
	}

	void EGameObject::save()
	{
		m_info->exportToFile();
	}

	EComponent* EGameObject::GetComponentByName(const std::string& name)
	{
		auto it = m_componentsMap.find(name);
		if (it != m_componentsMap.end())
			return (*it).second;
		return nullptr;
	}

	void EGameObject::AddChildGameObject(EGameObject* obj)
	{
		m_childGameObjectIdxMap[obj] = m_childGameObjects.size();
		m_childGameObjects.push_back(obj);
		m_childGameObjectMap[m_info->m_sGUID] = obj;
	}

	void EGameObject::AddComponent(EComponent* com)
	{
		if (m_componentsMap.find(com->m_sType) != m_componentsMap.end())
			return;
#ifdef _HAS_EDITOR_
		auto iter = m_componentsIndexMap.find(com);
		if (iter != m_componentsIndexMap.end())
			return;
		
		m_componentsIndexMap[com] = m_components.size();
		m_components.push_back(com);
#endif
		m_componentsMap[com->m_sType] = com;
	}

	EGameObject* EGameObject::CreateChildGameObject()
	{
		EGameObject* newGo = GameObjectMgr::instance()->createGameObject();
		AddChildGameObject(newGo);
		return newGo;
	}

	void EGameObject::destroy()
	{
		destoryImpl();
	}

	void EGameObject::destoryImpl()
	{
		for (auto com : m_componentsMap)
		{
			if (!com.second)continue;

			auto factory = ComponentFactoryManager::instance()->getFactoryByComType(com.second->m_sType);
			if (factory)
			{
				factory->destoryComponent(com.second);
			}
		}

		m_componentsMap.clear();
		for (auto go : m_childGameObjects)
		{
			if (!go)continue;

			go->destoryImpl();
		}
		m_childGameObjects.clear();
	}

	void EGameObject::removeComponent(EComponent* rhs)
	{
		if (rhs == nullptr)
			return;
#ifdef _HAS_EDITOR
		auto it = m_componentsIndexMap.find(rhs);
		if (it == m_componentsIndexMap.end())
			return;
		EComponent* removeCom = it->first;
		size_t		idx = it->second;
		m_componentsIndexMap.erase(it);
		auto iter = m_components.begin() + idx;
		m_components.erase(iter);
#endif
		
		auto factory = ComponentFactoryManager::instance()->getFactoryByComType(rhs->m_sType);
		if (factory == nullptr)
			return;

		factory->destoryComponent(rhs);
	}

	void EGameObject::removeChildGameObject(EGameObject* rhs)
	{
		if (rhs == nullptr)
			return;
		auto it = m_childGameObjectIdxMap.find(rhs);
		if (it == m_childGameObjectIdxMap.end())
			return;

		size_t idx = it->second;
		auto iter = m_childGameObjects.begin() + idx;
		m_childGameObjects.erase(iter);

		GameObjectMgr::instance()->destoryGameObject(rhs);
	}

	void EGameObject::setWorldPosition(const Vector3& pos)
	{
		
	}

	void EGameObject::setWorldScale(const Vector3& scale)
	{

	}

	void EGameObject::setWorldEulerRotation(const Vector3& rotation)
	{

	}

	void EGameObject::setWorldQuaternion(const Quaternion& rotation)
	{

	}

	void EGameObject::updateNode(bool childUpdate /*= true*/, bool notifyParent /*= true*/)
	{

	}
}