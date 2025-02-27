#include "ElainePrecompiledHeader.h"
#include "ElaineSceneNode.h"
#include "ElaineRenderQueue.h"


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

	const Matrix4x4& SceneNode::getWorldMatrix()
	{
		return m_WorldMat;
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

	void SceneNode::UpdateRenderQueue(RenderQueueSet* InRenderQueueSet)
	{
		for (auto&& CurrentRenderObj : m_RenderObjects)
		{
			if (CurrentRenderObj->IsVisible())
			{
				CurrentRenderObj->NotifyCurrentCamera(m_Creater->getMainCamera());
				if (!CurrentRenderObj->IsCulled())
				{
					CurrentRenderObj->UpdateRenderQueue(InRenderQueueSet);
				}
			}
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
		if (m_ChildSet.find(node) != m_ChildSet.end())
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
	void SceneNode::addRenderObject(RenderableObject* InObject)
	{
		if (InObject->m_Index != -1)
		{
			assert(false);
			return;
		}
		InObject->m_Index = m_RenderObjects.size();
		m_RenderObjects.push_back(InObject);
	}
	void SceneNode::removeRenderObject(RenderableObject* InObject)
	{
		if (InObject->m_Index == -1)
		{
			assert(false);
			return;
		}
		if (InObject->m_Index + 1 >= m_RenderObjects.size())
		{
			m_RenderObjects.resize(InObject->m_Index);
		}
		else
		{
			std::swap(m_RenderObjects[InObject->m_Index], m_RenderObjects.back());
			m_RenderObjects.resize(m_RenderObjects.size() - 1);
		}
		
		InObject->m_Index = -1;
	}
}