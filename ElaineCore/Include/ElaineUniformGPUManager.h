#pragma once
#include "ElaineMatrix4.h"

namespace Elaine
{
    struct CommonUniformBufferCPU
    {
        Matrix4x4 U_ViewMatrix;
        Matrix4x4 U_ProjectionMatrix;
        Matrix4x4 U_ViewProjectionMatrix;
        Vector4   U_CameraPosition;
        Vector4   U_CameraDirection;
        Vector4   U_LightDirection;
        Vector4   U_LightColor;
    };
}