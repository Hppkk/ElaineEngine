#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class ElaineCoreExport RenderableObject
	{
	public:
		RenderableObject();
		~RenderableObject();
		AxisAlignedBox			getWorldAABB();
		AxisAlignedBox			getLocalAABB();
		Vector3					getWorldPosition() const { return m_WorldPosition; }
		Vector3					getWorldScale() const { return m_WorldScale; }
		Quaternion				getWorldRotation() const { return m_WorldRotation; }
		void					setWorldPosition(const Vector3& pos);
		void					setWorldScale(const Vector3& scale);
		void					setWorldRotation(const Quaternion& rotation);
		Matrix4x4				getWorldMatrix();
		bool					isVisible() const { return m_Visible; }
		void					setVisible(bool vis) { m_Visible = vis; }
		void					updateWorldMatrix();
		void					updateBound();
	protected:
		AxisAlignedBox						m_BoundingBox;
		AxisAlignedBox						m_WorldBox;
		Vector3								m_WorldPosition;
		Vector3								m_WorldScale;
		Quaternion							m_WorldRotation;
		Matrix4x4							m_WorldMatrix;
		bool								m_Visible = true;
		bool								m_needUpdate = false;

	};
}