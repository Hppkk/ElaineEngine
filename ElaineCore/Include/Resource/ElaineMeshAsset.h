#pragma once
#include "Resource/ElaineAsset.h"
#include "math/ElaineAxisAlignedBox.h"
#include <cmath>

namespace Elaine
{
    inline JsonCpp MeshBoundsToJson(const AxisAlignedBox& Bounds)
    {
        if (Bounds.isNull()) return nullptr;
        if (Bounds.m_extent == AxisAlignedBox::Extent_Infinite) return "infinite";
        const auto& A = Bounds.getMin(); const auto& B = Bounds.getMax();
        return {{"min", {A.x, A.y, A.z}}, {"max", {B.x, B.y, B.z}}};
    }
    inline AxisAlignedBox MeshBoundsFromJson(const JsonCpp& Json)
    {
        AxisAlignedBox Bounds;
        if (Json.is_null()) return Bounds;
        if (Json == "infinite") { Bounds.setInfinite(); return Bounds; }
        auto A = Json.at("min").get<std::vector<float>>(); auto B = Json.at("max").get<std::vector<float>>();
        if (A.size() != 3 || B.size() != 3) throw std::runtime_error("Invalid Mesh bounds components");
        for (size_t I = 0; I < 3; ++I)
            if (!std::isfinite(A[I]) || !std::isfinite(B[I]) || A[I] > B[I]) throw std::runtime_error("Invalid Mesh bounds range");
        Bounds.setExtent(Vector3(A[0], A[1], A[2]), Vector3(B[0], B[1], B[2])); return Bounds;
    }
}
