#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineMatrix4.h"

namespace Elaine
{
    enum class ProjectionType
    {
        Prespective,
        Orthographic
    };

	class ElaineCoreExport Camera
	{
        enum ParamChangeType : uint8
        {
            ParamNone = 0,
            ParamViewMatrix = 1,
            ParamPosition = 1 << 1,
            ParamRotation = 1 << 2,
            ParamAxis = 1 << 3,
            ParamMagicNum = 0xff
        };
    public:
        Camera(const String& InName);
        void                    SetPosition(const Vector3& InPosition);
        void                    SetRotation(const Vector3& InRotation);
        void                    SetRotation(const Quaternion& InRotation);
        void                    LookAt(const Vector3& InTarget);
        void                    SetAspect(float InAspect);
        void                    SetProjectionType(ProjectionType InType);
        void                    SetFOV(float InFov);
        void                    SetNearPlane(float InNear);
        void                    SetFarPlane(float InFar);
        const Vector3&          GetPosition();
        const Quaternion&       GetRotation();
        const Vector3&          GetForward();
        const Vector3&          GetUp();
        const Vector3&          GetRight();
        float                   GetFOV() const;
        const Matrix4x4&        GetViewMatrix();
        const Matrix4x4&        GetProjMatrix();
        const Matrix4x4&        GetViewProjMatrix();
    private:
        void                    UpdateViewParams();
        void                    CalculateProjMatrix();
    private:
        String                  mName;
        Quaternion              mRotation{ Quaternion::IDENTITY };
        Quaternion              mInvRotation{ Quaternion::IDENTITY };
        Matrix4x4               mViewMatrix{ Matrix4x4::IDENTITY };
        Matrix4x4               mViewProjMatrix{ Matrix4x4::IDENTITY };
        Matrix4x4               mProjMatrix{ Matrix4x4::IDENTITY };
        Vector3                 mUp{ Vector3::UNIT_Y };
        Vector3                 mRight{ Vector3::UNIT_X };
        Vector3                 mForward{ Vector3::UNIT_Z };
        Vector3                 mPosition{ 0.0f, 0.0f, 0.0f };
        ProjectionType          mProjectionType{ ProjectionType::Prespective };
        float                   mFar{ 1000.0f };
        float                   mNear{ 0.1f };
        float                   mAspect {16.0f / 9.0f};
        float                   mFovy{ 60.0f };
        bool                    mbCacheOutOfData = true;
        bool                    mbCacheViewOutOfData = true;
        //std::mutex mViewMatrixMutex;
        static constexpr float MIN_FOV{ 10.0f };
        static constexpr float MAX_FOV{ 89.0f };
        static constexpr int   MAIN_VIEW_MATRIX_INDEX{ 0 };
	};
}