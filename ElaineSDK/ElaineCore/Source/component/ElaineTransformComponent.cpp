#include "ElainePrecompiledHeader.h"
#include "component/ElaineTransformComponent.h"
#include "math/ElaineTransform.h"

namespace Elaine
{
	std::string TransformComponent::m_sType = GetTypeStringName(TransformComponent);

	TransformComponentInfo::TransformComponentInfo()
	{
		m_pTransform = new Transform();
	}

	TransformComponentInfo::~TransformComponentInfo()
	{
		SAFE_DELETE(m_pTransform);
	}

	void TransformComponentInfo::exportDataImpl(cJSON* jsonNode)
	{
		auto pos = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "Position", pos);
		cJSON_AddItemToArray(pos, cJSON_CreateNumber(m_pTransform->m_position.x));
		cJSON_AddItemToArray(pos, cJSON_CreateNumber(m_pTransform->m_position.y));
		cJSON_AddItemToArray(pos, cJSON_CreateNumber(m_pTransform->m_position.z));

		auto scal = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "Scale", scal);
		cJSON_AddItemToArray(scal, cJSON_CreateNumber(m_pTransform->m_scale.x));
		cJSON_AddItemToArray(scal, cJSON_CreateNumber(m_pTransform->m_scale.y));
		cJSON_AddItemToArray(scal, cJSON_CreateNumber(m_pTransform->m_scale.z));

		auto rot = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "Rotation", rot);
		cJSON_AddItemToArray(rot, cJSON_CreateNumber(m_pTransform->m_rotation.x));
		cJSON_AddItemToArray(rot, cJSON_CreateNumber(m_pTransform->m_rotation.y));
		cJSON_AddItemToArray(rot, cJSON_CreateNumber(m_pTransform->m_rotation.z));

	}

	void TransformComponentInfo::importDataImpl(cJSON* jsonNode)
	{
		auto pos = cJSON_GetObjectItem(jsonNode, "Position");
		Vector3 pos_;
		pos_.x = (float)cJSON_GetArrayItem(pos, 0)->valuedouble;
		pos_.y = (float)cJSON_GetArrayItem(pos, 1)->valuedouble;
		pos_.z = (float)cJSON_GetArrayItem(pos, 2)->valuedouble;

		auto scale = cJSON_GetObjectItem(jsonNode, "Scale");
		Vector3 scale_;
		scale_.x = (float)cJSON_GetArrayItem(scale, 0)->valuedouble;
		scale_.y = (float)cJSON_GetArrayItem(scale, 1)->valuedouble;
		scale_.z = (float)cJSON_GetArrayItem(scale, 2)->valuedouble;

		auto rot = cJSON_GetObjectItem(jsonNode, "Rotation");
		Vector3 rot_;
		rot_.x = (float)cJSON_GetArrayItem(rot, 0)->valuedouble;
		rot_.y = (float)cJSON_GetArrayItem(rot, 1)->valuedouble;
		rot_.z = (float)cJSON_GetArrayItem(rot, 2)->valuedouble;

	}

	TransformComponent::TransformComponent()
	{
		m_info = static_cast<TransformComponentInfo*>(m_pComInfo);
		registerTickTime(beginFrame);
		registerComNeedTick();
	}

	TransformComponent::~TransformComponent()
	{
		
	}

	Matrix4x4 TransformComponent::getWorldMatrix()
	{
		return m_info->m_pTransform->getMatrix();
	}

	Vector3	  TransformComponent::getWorldScale()
	{
		return m_info->m_pTransform->m_scale;
	}

	Vector3	  TransformComponent::getWorldPosition()
	{
		return m_info->m_pTransform->m_position;
	}

	Quaternion TransformComponent::getWorldRotation()
	{
		return m_info->m_pTransform->m_rotation;
	}

	void TransformComponent::setWorldScale(const Vector3& rhs)
	{
		m_info->m_pTransform->m_scale = rhs;
	}
	void TransformComponent::setWorldPosition(const Vector3& rhs)
	{
		m_info->m_pTransform->m_position = rhs;
	}
	void TransformComponent::setWorldRotation(const Quaternion& rhs)
	{
		m_info->m_pTransform->m_rotation = rhs;
	}

	void TransformComponent::beginFrameTick(float dt)
	{

	}

}
