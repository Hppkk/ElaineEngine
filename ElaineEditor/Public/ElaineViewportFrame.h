#pragma once
#include "ElaineCameraState.h"
#include <cstdint>

namespace Editor
{
    // Attached to the pixels, not to the current UI tick. Published with its slot.
    struct ViewportFrameData
    {
        Elaine::CameraState Camera;
        Elaine::CameraMatrices Matrices;
        uint64_t FrameNumber = 0;
        bool Valid = false;
    };

    inline void ToImGuizmoMatrix(const Elaine::Matrix4x4& Matrix, float (&Data)[16])
    {
        for (size_t Row = 0; Row != 4; ++Row)
            for (size_t Column = 0; Column != 4; ++Column) Data[Column * 4 + Row] = Matrix[Row][Column];
    }
    inline Elaine::Matrix4x4 FromImGuizmoMatrix(const float* Data)
    {
        Elaine::Matrix4x4 Result;
        for (size_t Row = 0; Row != 4; ++Row)
            for (size_t Column = 0; Column != 4; ++Column) Result[Row][Column] = Data[Column * 4 + Row];
        return Result;
    }
}
