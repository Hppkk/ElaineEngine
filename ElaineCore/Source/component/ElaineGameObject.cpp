#include "ElainePrecompiledHeader.h"
#include "component/ElaineGameObject.h"
#include "component/ElaineComponent.h"

namespace Elaine
{
	EGameObjectInfo::EGameObjectInfo()
	{

	}

	EGameObjectInfo::EGameObjectInfo(const std::string& path) :ResourceBase(path)
	{

	}

	EGameObjectInfo::~EGameObjectInfo()
	{

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

	}



	EGameObject::EGameObject()
	{

	}

	EGameObject::EGameObject(const std::string& name)
	{

	}

	EGameObject::~EGameObject()
	{

	}

	void EGameObject::init(EGameObjectInfo* info)
	{

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

	}

	void EGameObject::AddComponents(EComponent* com)
	{

	}

	EGameObject* EGameObject::CreateChildGameObject()
	{
		return nullptr;
	}

	void EGameObject::destroy()
	{

	}

	void EGameObject::removeComponent(EComponent* rhs)
	{
		if (rhs == nullptr)
			return;

		auto it = m_componentsIndexMap.find(rhs);
		if (it == m_componentsIndexMap.end())
			return;
		EComponent* removeCom = it->first;
		size_t		idx = it->second;
		m_componentsIndexMap.erase(it);
		auto iter = m_components.begin() + idx;
		m_components.erase(iter);
		
		auto factory = ComponentFactoryManager::instance()->getFactoryByComType(rhs->m_sType);
		if (factory == nullptr)
			return;

		factory->destoryComponent(removeCom);
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

	}
}