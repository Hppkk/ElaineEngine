#pragma once

#include "imgui/ImGuizmo/ImGuizmo.h"

namespace Elaine { class Actor; class Matrix4x4; }

namespace Editor
{
    enum class GizmoType : unsigned int
    {
        Transform = 1u << 0,
        Grid      = 1u << 1,
        Bounds    = 1u << 2,
    };

    inline GizmoType operator|(GizmoType A, GizmoType B)
    {
        return static_cast<GizmoType>(static_cast<unsigned int>(A) | static_cast<unsigned int>(B));
    }

    class EditorGizmoManager
    {
    public:
        EditorGizmoManager();
        ~EditorGizmoManager();

        void Initialize();
        void Shutdown();

        void SetVisible(GizmoType Type, bool Visible);
        bool IsVisible(GizmoType Type) const;
        unsigned int GetVisibilityMask() const { return mVisibilityMask; }

        void SetOperation(ImGuizmo::OPERATION Operation) { mOperation = Operation; }
        ImGuizmo::OPERATION GetOperation() const { return mOperation; }
        void SetMode(ImGuizmo::MODE Mode) { mMode = Mode; }
        ImGuizmo::MODE GetMode() const { return mMode; }
        float GetSize() const { return mSize; }
        void SetSize(float Size);

        void DrawSettings();
        bool DrawTransform(Elaine::Actor* Actor, const Elaine::Matrix4x4& View,
            const Elaine::Matrix4x4& Projection, float X, float Y, float Width, float Height);

    private:
        unsigned int mVisibilityMask = static_cast<unsigned int>(GizmoType::Transform) |
            static_cast<unsigned int>(GizmoType::Grid) |
            static_cast<unsigned int>(GizmoType::Bounds);
        ImGuizmo::OPERATION mOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE mMode = ImGuizmo::WORLD;
        float mSize = 0.1f;
        bool mInitialized = false;
    };
}
