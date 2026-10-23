#include "ElainePrecompiledHeader.h"
#include "ElaineCamera.h"

namespace Elaine
{
    Camera::Camera(const String& Name) : mName(Name) { UpdateMatrices(); }
    void Camera::SetState(const CameraState& State) { mState = ValidateCameraState(State); mDirty = true; }
    void Camera::SetPosition(const Vector3& Value) { auto State = mState; State.Position = Value; SetState(State); }
    void Camera::SetRotation(const Vector3& Value) { SetRotation(Quaternion::fromEulerZYX(Value)); }
    void Camera::SetRotation(const Quaternion& Value) { auto State = mState; State.Rotation = Value; SetState(State); }
    void Camera::LookAt(const Vector3& Target) { auto State = mState; if (SetCameraLookAt(State, Target)) SetState(State); }
    void Camera::SetAspect(float Value) { auto State = mState; State.Aspect = Value; SetState(State); }
    void Camera::SetProjectionType(ProjectionType Value) { auto State = mState; State.Projection = Value; SetState(State); }
    void Camera::SetFOV(float Value) { auto State = mState; State.FOV = Value; SetState(State); }
    void Camera::SetNearPlane(float Value) { auto State = mState; State.Near = Value; SetState(State); }
    void Camera::SetFarPlane(float Value) { auto State = mState; State.Far = Value; SetState(State); }
    void Camera::UpdateMatrices() { if (mDirty) { mMatrices = BuildCameraMatrices(mState); mDirty = false; } }
    const Vector3& Camera::GetForward() { UpdateMatrices(); return mMatrices.Forward; }
    const Vector3& Camera::GetUp() { UpdateMatrices(); return mMatrices.Up; }
    const Vector3& Camera::GetRight() { UpdateMatrices(); return mMatrices.Right; }
    const Matrix4x4& Camera::GetViewMatrix() { UpdateMatrices(); return mMatrices.View; }
    const Matrix4x4& Camera::GetProjMatrix() { UpdateMatrices(); return mMatrices.Projection; }
    const Matrix4x4& Camera::GetViewProjMatrix() { UpdateMatrices(); return mMatrices.ViewProjection; }
}
