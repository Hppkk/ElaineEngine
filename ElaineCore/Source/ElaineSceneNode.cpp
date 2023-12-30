#include "ElainePrecompiledHeader.h"


namespace Elaine
{
	SceneNode::SceneNode(SceneManager* mgr, String name)
	{
		m_Creater = mgr;
		m_Name = name;
	}

	SceneNode::~SceneNode()
	{

	}

	void SceneNode::setWorldPosition(const Vector3& pos)
	{
		m_WorldPosition = pos;
		update();
	}

	void SceneNode::setWorldScale(const Vector3& scale)
	{
		m_WorldScale = scale;
		update();
	}

	void SceneNode::setWorldRotation(const Vector3& rotation)
	{
		m_WorldRotation = rotation;
		update();
	}

	void SceneNode::setWorldQuaternion(const Quaternion& rotation)
	{
		m_WorldQuaternion = rotation;
		update();
	}

	void SceneNode::update(bool updateChild /*= true*/, bool notifyParent /*= true*/)
	{
		updateBindingBox();
	}

	void SceneNode::updateBindingBox()
	{

	}

	void SceneNode::findVisibilityObject()
	{
		for (auto child : m_ChildSet)
		{
			if (!child->isVisible())
				continue;

			child->findVisibilityObject();
		}
	}

	SceneNode* SceneNode::createChild()
	{
		SceneNode* pNode = m_Creater->createSceneNode();
		attachChildNode(pNode);
		return pNode;
	}

	void SceneNode::attachChildNode(SceneNode* node)
	{
		if (m_ChildSet.find(node) == m_ChildSet.end())
			return;

		m_ChildSet.insert(node);
		updateBindingBox();
	}

	void SceneNode::detachChildNode(SceneNode* node)
	{
		if (m_ChildSet.find(node) == m_ChildSet.end())
			return;

		m_ChildSet.erase(node);
		updateBindingBox();
	}
}