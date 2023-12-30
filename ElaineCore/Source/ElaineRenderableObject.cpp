#include "ElainePrecompiledHeader.h"
#include "ElaineRenderableObject.h"

namespace Elaine
{
	RenderableObject::RenderableObject()
	{

	}

	RenderableObject::~RenderableObject()
	{

	}

	AxisAlignedBox RenderableObject::getWorldAABB()
	{
		return m_WorldBox;
	}

	AxisAlignedBox RenderableObject::getLocalAABB()
	{
		return m_BoundingBox;
	}

	void RenderableObject::setWorldPosition(const Vector3& pos)
	{
		m_WorldPosition = pos;
		m_needUpdate = true;
		updateBound();
	}

	void RenderableObject::setWorldScale(const Vector3& scale)
	{
		m_WorldScale = scale;
		m_needUpdate = true;
		updateBound();
	}

	void RenderableObject::setWorldRotation(const Quaternion& rotation)
	{
		m_WorldRotation = rotation;
		m_needUpdate = true;
		updateBound();
	}

	Matrix4x4 RenderableObject::getWorldMatrix()
	{
		return m_WorldMatrix;
	}

	void RenderableObject::updateWorldMatrix()
	{
		if (!m_needUpdate)
			return;

		m_WorldMatrix.makeTransform(m_WorldPosition, m_WorldScale, m_WorldRotation);
		m_needUpdate = false;
	}

	void RenderableObject::updateBound()
	{
		updateWorldMatrix();
		m_WorldBox = m_BoundingBox;
		m_WorldBox.transformAffine(m_WorldMatrix);
	}
}