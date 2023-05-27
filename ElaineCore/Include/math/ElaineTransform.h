#pragma once
#include "ElaineVector3.h"
#include "ElaineMatrix4.h"

namespace Elaine
{
	class Transform
	{
	public:
        Vector3    m_position       = Vector3::ZERO;
        Vector3    m_scale          = Vector3::UNIT_SCALE;
        Quaternion m_rotation       = Quaternion::IDENTITY;
    public:
        Transform() = default;
        Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale) :
            m_position{ position }, m_scale{ scale }, m_rotation{ rotation }
        {
        }

        Matrix4x4 getMatrix() const
        {
            Matrix4x4 temp;
            temp.makeTransform(m_position, m_scale, m_rotation);
            return temp;
        }
	};
}