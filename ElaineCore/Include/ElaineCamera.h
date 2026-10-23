#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineCameraState.h"

namespace Elaine
{
    class ElaineCoreExport Camera
    {
    public:
        Camera(const String& InName);
        void SetState(const CameraState& State);
        const CameraState& GetState() const { return mState; }
        void SetPosition(const Vector3& Position);
        void SetRotation(const Vector3& Rotation);
        void SetRotation(const Quaternion& Rotation);
        void LookAt(const Vector3& Target);
        void SetAspect(float Aspect);
        void SetProjectionType(ProjectionType Type);
        void SetFOV(float FOV);
        void SetNearPlane(float Near);
        void SetFarPlane(float Far);
        const Vector3& GetPosition() { return mState.Position; }
        const Quaternion& GetRotation() { return mState.Rotation; }
        const Vector3& GetForward();
        const Vector3& GetUp();
        const Vector3& GetRight();
        float GetFOV() const { return mState.FOV; }
        const Matrix4x4& GetViewMatrix();
        const Matrix4x4& GetProjMatrix();
        const Matrix4x4& GetViewProjMatrix();
    private:
        void UpdateMatrices();
        String mName;
        CameraState mState;
        CameraMatrices mMatrices;
        bool mDirty = true;
    };
}
