#pragma once
#include "ElaineVector3.h"
#include "ElaineMatrix4.h"
#include "ElaineQuaternion.h"

namespace Elaine
{
	class Transform
	{
    public:
        Transform() = default;
        Transform(const Vector3& InPosition, const Quaternion& InRotation, const Vector3& InScale) :
            mPosition{ InPosition }, mScale{ InScale }, mRotation{ InRotation }
        {
        }

        const Matrix4x4& GetMatrix() const
        {
            if (mDirty)
            {
                mMatrix.makeTransform(mPosition, mScale, mRotation);
                mDirty = false;
            }

            return mMatrix;
        }
    public:
        mutable bool mDirty = true;
        mutable Matrix4x4 mMatrix;
        Vector3 mPosition = Vector3::ZERO;
        Vector3 mScale = Vector3::UNIT_SCALE;
        Quaternion mRotation = Quaternion::IDENTITY;
	};
}