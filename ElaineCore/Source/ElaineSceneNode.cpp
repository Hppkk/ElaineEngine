#include "ElainePrecompiledHeader.h"


namespace Elaine
{
	SceneNode::SceneNode()
	{

	}

	SceneNode::~SceneNode()
	{

	}

	void SceneNode::setWorldPosition(const Vector3& pos)
	{
		m_WorldPosition = pos;
	}

	void SceneNode::setWorldScale(const Vector3& scale)
	{
		m_WorldScale = scale;
	}

	void SceneNode::setWorldRotation(const Vector3& rotation)
	{
		m_WorldRotation = rotation;
	}

	void SceneNode::setWorldQuaternion(const Quaternion& rotation)
	{
		m_WorldQuaternion = rotation;
	}

	void SceneNode::update(bool updateChild /*= true*/, bool notifyParent /*= true*/)
	{

	}
}