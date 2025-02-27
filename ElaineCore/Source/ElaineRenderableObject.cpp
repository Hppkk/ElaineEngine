#include "ElainePrecompiledHeader.h"
#include "ElaineRenderableObject.h"

namespace Elaine
{
	RenderableObject::RenderableObject(SceneNode* InParent, SceneManager* InSceneMgr)
		:m_SceneManager(InSceneMgr)
	{
		m_SceneNode = InParent->createChild();
		m_SceneNode->addRenderObject(this);
	}

	RenderableObject::~RenderableObject()
	{
		m_SceneNode->removeRenderObject(this);
		m_SceneNode->getParentNode()->detachChildNode(m_SceneNode);
		m_SceneManager->destroySceneNode(m_SceneNode);
		m_SceneNode = nullptr;
	}

	const AxisAlignedBox& RenderableObject::GetWorldAABB()
	{
		return m_SceneNode->getWorldAABB();
	}

	const AxisAlignedBox& RenderableObject::GetLocalAABB()
	{
		return m_SceneNode->getLocalAABB();
	}

	const Vector3& RenderableObject::GetWorldPosition() const
	{
		return m_SceneNode->getWorldPosition();
	}

	const Vector3& RenderableObject::GetWorldScale() const
	{
		return m_SceneNode->getWorldScale();
	}

	const Quaternion& RenderableObject::GetWorldRotation() const
	{
		return m_SceneNode->getWorldQuaternion();
	}

	void RenderableObject::SetWorldPosition(const Vector3& pos)
	{
		m_SceneNode->setWorldPosition(pos);
	}

	void RenderableObject::SetWorldScale(const Vector3& scale)
	{
		m_SceneNode->setWorldScale(scale);
	}

	void RenderableObject::SetWorldRotation(const Quaternion& rotation)
	{
		m_SceneNode->setWorldQuaternion(rotation);
	}

	void RenderableObject::SetVisible(bool InVal)
	{
		if (InVal == m_bVisible)
			return;
		m_bVisible = InVal;
		m_SceneNode->setVisible(InVal);
	}

	const Matrix4x4& RenderableObject::GetWorldMatrix()
	{
		return m_SceneNode->getWorldMatrix();
	}

	void RenderableObject::UpdateWorldMatrix()
	{

	}

	void RenderableObject::UpdateBound()
	{

	}
}