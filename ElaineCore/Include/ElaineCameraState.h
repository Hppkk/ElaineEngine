#pragma once
#include "math/ElaineMatrix4.h"
#include "math/ElaineQuaternion.h"
#include <algorithm>
#include <cmath>

namespace Elaine
{
    enum class ProjectionType { Prespective, Orthographic };

    // Value-only state shared by GameThread snapshots and the render camera.
    // Local +Z is forward, +Y is up; projection depth is [0, 1].
    struct CameraState
    {
        Vector3 Position = Vector3::ZERO;
        Quaternion Rotation = Quaternion::IDENTITY;
        float FOV = 60.0f, Near = 0.1f, Far = 1000.0f, Aspect = 16.0f / 9.0f;
        ProjectionType Projection = ProjectionType::Prespective;
    };

    inline Quaternion NormalizeCameraRotation(Quaternion Rotation)
    {
        const float Length = Rotation.length();
        if (!std::isfinite(Length) || Length < 0.000001f) return Quaternion::IDENTITY;
        return Rotation / Length;
    }

    inline CameraState ValidateCameraState(CameraState State)
    {
        if (!std::isfinite(State.Position.x) || !std::isfinite(State.Position.y) || !std::isfinite(State.Position.z))
            State.Position = Vector3::ZERO;
        State.Rotation = NormalizeCameraRotation(State.Rotation);
        State.FOV = std::isfinite(State.FOV) ? std::clamp(State.FOV, 1.0f, 179.0f) : 60.0f;
        State.Near = std::isfinite(State.Near) ? (std::max)(State.Near, 0.0001f) : 0.1f;
        State.Far = std::isfinite(State.Far) ? (std::max)(State.Far, State.Near + (std::max)(0.01f, State.Near * 0.001f)) : State.Near + 1000.0f;
        State.Aspect = std::isfinite(State.Aspect) && State.Aspect > 0.000001f ? State.Aspect : 1.0f;
        if (State.Projection != ProjectionType::Prespective && State.Projection != ProjectionType::Orthographic)
            State.Projection = ProjectionType::Prespective;
        return State;
    }

    struct CameraMatrices
    {
        Matrix4x4 View, Projection, ViewProjection, InverseViewProjection;
        Vector3 Right, Up, Forward;
    };

    inline CameraMatrices BuildCameraMatrices(const CameraState& Input)
    {
        const auto State = ValidateCameraState(Input);
        CameraMatrices Result;
        Result.Right = State.Rotation * Vector3::UNIT_X;
        Result.Up = State.Rotation * Vector3::UNIT_Y;
        Result.Forward = State.Rotation * Vector3::UNIT_Z;
        const auto& R = Result.Right; const auto& U = Result.Up; const auto& F = Result.Forward;
        Result.View = Matrix4x4(R.x, R.y, R.z, -R.dotProduct(State.Position),
            U.x, U.y, U.z, -U.dotProduct(State.Position),
            F.x, F.y, F.z, -F.dotProduct(State.Position), 0, 0, 0, 1);
        const float Y = 1.0f / std::tan(Degree(State.FOV).ValueRadians() * 0.5f);
        const float Depth = State.Far - State.Near;
        if (State.Projection == ProjectionType::Prespective)
            Result.Projection = Matrix4x4(Y / State.Aspect, 0, 0, 0, 0, Y, 0, 0,
                0, 0, State.Far / Depth, -State.Far * State.Near / Depth, 0, 0, 1, 0);
        else
            Result.Projection = Matrix4x4(Y / (State.Aspect * State.Near), 0, 0, 0,
                0, Y / State.Near, 0, 0, 0, 0, 1.0f / Depth, -State.Near / Depth, 0, 0, 0, 1);
        Result.ViewProjection = Result.Projection * Result.View;
        Result.InverseViewProjection = Result.ViewProjection.inverse();
        return Result;
    }

    inline bool SetCameraLookAt(CameraState& State, const Vector3& Target)
    {
        Vector3 Forward = Target - State.Position;
        const float Length = Forward.length();
        if (!std::isfinite(Length) || Length < 0.000001f) return false;
        Forward = Forward / Length;
        const Vector3 ReferenceUp = std::abs(Forward.y) > 0.999f ? Vector3::UNIT_Z : Vector3::UNIT_Y;
        Vector3 Right = ReferenceUp.crossProduct(Forward).normalisedCopy();
        Vector3 Up = Forward.crossProduct(Right).normalisedCopy();
        State.Rotation = NormalizeCameraRotation(Quaternion(Right, Up, Forward));
        return true;
    }

    // Vulkan uses a downward-positive framebuffer Y with our positive viewport height.
    // Apply this once to render-view parameters, never to logical camera/gizmo data.
    inline CameraMatrices AdjustCameraForRendering(CameraMatrices Matrices, bool FlipY)
    {
        if (FlipY)
            for (size_t Column = 0; Column != 4; ++Column) Matrices.Projection[1][Column] *= -1.0f;
        Matrices.ViewProjection = Matrices.Projection * Matrices.View;
        Matrices.InverseViewProjection = Matrices.ViewProjection.inverse();
        return Matrices;
    }
}
