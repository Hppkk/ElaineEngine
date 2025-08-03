#include "ElainePrecompiledHeader.h"

namespace Elaine
{

    Camera::Camera(const String& InName)
        : mName(InName)
    {
        UpdateViewParams();
    }

    void Camera::SetPosition(const Vector3& InPosition)
    {
        if (InPosition == mPosition)
            return;

        mPosition = InPosition;
        mbCacheViewOutOfData = true;
        mbCacheOutOfData = true;
    }

    void Camera::SetRotation(const Vector3& InRotation)
    {
        mRotation = Quaternion::fromEulerZYX(InRotation);
        mbCacheViewOutOfData = true;
        mbCacheOutOfData = true;
    }

    void Camera::SetRotation(const Quaternion& InRotation)
    {
        if (InRotation == mRotation)
            return;

        mRotation = InRotation;
        mbCacheViewOutOfData = true;
        mbCacheOutOfData = true;
    }

    void Camera::LookAt(const Vector3& InTarget)
    {
        mForward = (InTarget - mPosition).normalisedCopy();
        mRight = (mUp.crossProduct(mForward)).normalisedCopy();
        mUp = (mForward.crossProduct(mRight)).normalisedCopy();
        mViewMatrix = Matrix4x4(mRight[0], mUp[0], mForward[0], -mRight * mPosition,
                                mRight[1], mUp[1], mForward[1], -mUp * mPosition,
                                mRight[2], mUp[2], mForward[2], -mForward * mPosition,
                                0.0f, 0.0f, 0.0f, 1.0f);

        mbCacheOutOfData = true;
    }

    const Matrix4x4& Camera::GetViewMatrix()
    {
        if (mbCacheViewOutOfData)
        {
            UpdateViewParams();
        }

        return mViewMatrix;
    }

    const Matrix4x4& Camera::GetProjMatrix()
    {
        if (mbCacheViewOutOfData)
        {
            UpdateViewParams();
        }
        else if (mbCacheOutOfData)
        {
            CalculateProjMatrix();
        }

        return mProjMatrix;
    }

    const Matrix4x4& Camera::GetViewProjMatrix()
    {
        if (mbCacheViewOutOfData)
        {
            UpdateViewParams();
        }
        else if (mbCacheOutOfData)
        {
            CalculateProjMatrix();
        }

        return mViewProjMatrix;
    }

    void Camera::UpdateViewParams()
    {
        if (!mbCacheViewOutOfData)
            return;
        
        float x = mRotation.x;
        float y = mRotation.y;
        float z = mRotation.z;
        float w = mRotation.w;


        mRight = (mRotation * Vector3::UNIT_X).normalisedCopy();
        mUp = (mRotation * -Vector3::UNIT_Y).normalisedCopy();
        mForward = (mRotation * -Vector3::UNIT_Z).normalisedCopy();
        //mRight = Vector3(1.0f - 2.0f * y * y - 2.0f * z * z,
        //    2.0f * x * y + 2.0f * w * z,
        //    2.0f * x * z - 2.0f * w * y);

        //mUp = Vector3(2 * x * y - 2 * w * z,
        //    1 - 2 * x * x - 2 * z * z,
        //    2 * y * z + 2 * w * x);

        //mForward = Vector3(2 * x * z + 2 * w * y,
        //    2 * y * z - 2 * w * x,
        //    2 * x * x - 2 * y * y);

        Matrix4x4 TranslationMat = Matrix4x4::IDENTITY;
        TranslationMat[0][3] = -mPosition[0];
        TranslationMat[1][3] = -mPosition[1];
        TranslationMat[2][3] = -mPosition[2];


        mViewMatrix = Matrix4x4(mRight[0], mUp[0], mForward[0], 0.0f,
                                mRight[1], mUp[1], mForward[1], 0.0f,
                                mRight[2], mUp[2], mForward[2], 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f).transpose() * TranslationMat;

        mbCacheViewOutOfData = false;

        CalculateProjMatrix();

    }

    void Camera::CalculateProjMatrix()
    {
        if (!mbCacheOutOfData)
            return;

        mbCacheOutOfData = false;

        Degree Fovy(mFovy);
        float RadFovy = Fovy.ValueRadians();
        if (mProjectionType == ProjectionType::Prespective)
        {

            float xx = 1.0f / (mAspect * Math::tan(RadFovy / 2.0f));
            float yy = 1.0f / Math::tan(RadFovy / 2.0f);
            float zz = mFar / (mFar - mNear);
            float zw = -(mFar * mNear) / (mFar - mNear);
            mProjMatrix = Matrix4x4(xx, 0.0f, 0.0f, 0.0f,
                                    0.0f, yy, 0.0f, 0.0f,
                                    0.0f, 0.0f, zz, zw,
                                    0.0f, 0.0f, 1.0f, 0.0f);
        }
        else
        {
            float h = 2 * mNear * Math::tan(RadFovy / 2.0f);
            float w = mAspect * h;
            float l = -w / 2.0f;
            float r = w / 2.0f;
            float b = -h / 2.0f;
            float t = h / 2.0f;
            mProjMatrix = Matrix4x4(2.0f / (r - l), 0.0f, 0.0f, -(r + l) / (r - l),
                                    0.0f, -2.0f / (t - b), 0.0f, -(t + b) / (t - b),
                                    0.0f, 0.0f, 1.0f / (mFar - mNear), -mNear / (mFar - mNear),
                                    0.0f, 0.0f, 0.0f, 1.0f);
        }

        mViewProjMatrix = mProjMatrix * mViewMatrix;
    }

    void Camera::SetProjectionType(ProjectionType InType)
    {
        if (mProjectionType == InType)
            return;

        mProjectionType = InType;
        mbCacheOutOfData = true;
    }

    void Camera::SetFOV(float InFov)
    {
        if (InFov == mFovy)
            return;

        mFovy = InFov;
        mbCacheOutOfData = true;
    }

    const Vector3& Camera::GetPosition()
    {
        if (mbCacheOutOfData)
        {
            UpdateViewParams();
        }

        return mPosition;
    }

    const Quaternion& Camera::GetRotation()
    {
        if (mbCacheOutOfData)
        {
            UpdateViewParams();
        }

        return mRotation;
    }

    const Vector3& Camera::GetForward()
    {
        if (mbCacheViewOutOfData)
        {
            UpdateViewParams();
        }

        return mForward;
    }

    const Vector3& Camera::GetUp()
    {
        if (mbCacheViewOutOfData)
        {
            UpdateViewParams();
        }

        return mUp;
    }

    const Vector3& Camera::GetRight()
    {
        if (mbCacheViewOutOfData)
        {
            UpdateViewParams();
        }

        return mRight;
    }

    float Camera::GetFOV() const
    {
        return mFovy;
    }

    void Camera::SetAspect(float InAspect)
    {
        if (mAspect == InAspect)
            return;

        mAspect = InAspect;
        mbCacheOutOfData = true;
    }
}