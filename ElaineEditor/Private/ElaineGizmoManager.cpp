#include "ElaineGizmoManager.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "ElaineActor.h"
#include "math/ElaineAxisAlignedBox.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
    Editor::EditorGizmoManager* GManager = nullptr;

    void ClearSettings(ImGuiContext*, ImGuiSettingsHandler*) {}

    void* OpenSettings(ImGuiContext*, ImGuiSettingsHandler*, const char* Name)
    {
        return (GManager && Name && std::strcmp(Name, "Viewport") == 0) ? GManager : nullptr;
    }

    void ReadSettings(ImGuiContext*, ImGuiSettingsHandler*, void* Entry, const char* Line)
    {
        auto* Manager = static_cast<Editor::EditorGizmoManager*>(Entry);
        unsigned int Mask = 0;
        int Operation = 0;
        int Mode = 1;
        float Size = 0.1f;
        if (std::sscanf(Line, "Visibility=%u", &Mask) == 1)
            Manager->SetVisible(Editor::GizmoType::Transform, (Mask & 1u) != 0);
        else if (std::sscanf(Line, "Operation=%d", &Operation) == 1)
            Manager->SetOperation(static_cast<ImGuizmo::OPERATION>(Operation));
        else if (std::sscanf(Line, "Mode=%d", &Mode) == 1)
            Manager->SetMode(static_cast<ImGuizmo::MODE>(Mode));
        else if (std::sscanf(Line, "Size=%f", &Size) == 1)
            Manager->SetSize(Size);
        if (std::sscanf(Line, "Visibility=%u", &Mask) == 1)
        {
            Manager->SetVisible(Editor::GizmoType::Grid, (Mask & 2u) != 0);
            Manager->SetVisible(Editor::GizmoType::Bounds, (Mask & 4u) != 0);
        }
    }

    void ApplySettings(ImGuiContext*, ImGuiSettingsHandler*) {}

    void WriteSettings(ImGuiContext*, ImGuiSettingsHandler*, ImGuiTextBuffer* Buffer)
    {
        if (!GManager) return;
        Buffer->appendf("[ElaineGizmo][Viewport]\n");
        Buffer->appendf("Visibility=%u\n", GManager->GetVisibilityMask());
        Buffer->appendf("Operation=%d\n", static_cast<int>(GManager->GetOperation()));
        Buffer->appendf("Mode=%d\n", static_cast<int>(GManager->GetMode()));
        Buffer->appendf("Size=%.4f\n", GManager->GetSize());
        Buffer->appendf("\n");
    }

}

namespace Editor
{
    EditorGizmoManager::EditorGizmoManager() = default;

    EditorGizmoManager::~EditorGizmoManager()
    {
        Shutdown();
    }

    void EditorGizmoManager::Initialize()
    {
        if (mInitialized) return;
        GManager = this;
        ImGuiSettingsHandler Handler{};
        Handler.TypeName = "ElaineGizmo";
        Handler.TypeHash = ImHashStr(Handler.TypeName);
        Handler.ClearAllFn = ClearSettings;
        Handler.ReadOpenFn = OpenSettings;
        Handler.ReadLineFn = ReadSettings;
        Handler.ApplyAllFn = ApplySettings;
        Handler.WriteAllFn = WriteSettings;
        ImGui::AddSettingsHandler(&Handler);
        mInitialized = true;
    }

    void EditorGizmoManager::Shutdown()
    {
        if (!mInitialized) return;
        if (ImGui::GetIO().IniFilename)
            ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
        ImGui::RemoveSettingsHandler("ElaineGizmo");
        if (GManager == this) GManager = nullptr;
        mInitialized = false;
    }

    void EditorGizmoManager::SetVisible(GizmoType Type, bool Visible)
    {
        if (Visible) mVisibilityMask |= static_cast<unsigned int>(Type);
        else mVisibilityMask &= ~static_cast<unsigned int>(Type);
    }

    bool EditorGizmoManager::IsVisible(GizmoType Type) const
    {
        return (mVisibilityMask & static_cast<unsigned int>(Type)) != 0;
    }

    void EditorGizmoManager::SetSize(float Size)
    {
        mSize = std::max(0.02f, std::min(Size, 0.5f));
    }

    void EditorGizmoManager::DrawSettings()
    {
        bool Transform = IsVisible(GizmoType::Transform);
        bool Grid = IsVisible(GizmoType::Grid);
        bool Bounds = IsVisible(GizmoType::Bounds);
        bool Changed = false;
        if (ImGui::Checkbox("Transform Gizmo", &Transform)) { SetVisible(GizmoType::Transform, Transform); Changed = true; }
        ImGui::SameLine();
        if (ImGui::Checkbox("Grid", &Grid)) { SetVisible(GizmoType::Grid, Grid); Changed = true; }
        ImGui::SameLine();
        if (ImGui::Checkbox("Bounds", &Bounds)) { SetVisible(GizmoType::Bounds, Bounds); Changed = true; }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(90.0f);
        Changed |= ImGui::SliderFloat("Size", &mSize, 0.02f, 0.5f, "%.2f");
        ImGui::SameLine();
        if (ImGui::RadioButton("Move", mOperation == ImGuizmo::TRANSLATE)) { mOperation = ImGuizmo::TRANSLATE; Changed = true; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", mOperation == ImGuizmo::ROTATE)) { mOperation = ImGuizmo::ROTATE; Changed = true; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", mOperation == ImGuizmo::SCALE)) { mOperation = ImGuizmo::SCALE; Changed = true; }
        ImGui::SameLine();
        bool Local = mMode == ImGuizmo::LOCAL;
        if (ImGui::RadioButton("Local", Local)) { mMode = ImGuizmo::LOCAL; Changed = true; }
        ImGui::SameLine();
        bool World = mMode == ImGuizmo::WORLD;
        if (ImGui::RadioButton("World", World)) { mMode = ImGuizmo::WORLD; Changed = true; }
        if (Changed) ImGui::MarkIniSettingsDirty();
    }

    bool EditorGizmoManager::DrawTransform(Elaine::Actor* Actor, const Elaine::Matrix4x4& View,
        const Elaine::Matrix4x4& Projection, float X, float Y, float Width, float Height)
    {
        if (!Actor || (!IsVisible(GizmoType::Transform) && !IsVisible(GizmoType::Bounds)) || Width <= 1.0f || Height <= 1.0f)
            return false;

        float ViewData[16], ProjectionData[16], ModelData[16];
        View.toData(ViewData);
        Projection.toData(ProjectionData);
        Elaine::Matrix4x4 World = Actor->GetWorldMatrix();
        World.toData(ModelData);
        ImGuizmo::SetRect(X, Y, Width, Height);
        ImGuizmo::SetGizmoSizeClipSpace(mSize);
        ImGuizmo::OPERATION Operation = IsVisible(GizmoType::Transform) ? mOperation : ImGuizmo::BOUNDS;
        float LocalBounds[6] = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
        if (IsVisible(GizmoType::Bounds))
        {
            const Elaine::AxisAlignedBox Bounds = Actor->GetBoundingBox();
            const Elaine::Matrix4x4 InverseWorld = World.inverse();
            const Elaine::Vector3 Min = Bounds.isNull() ? Elaine::Vector3(-0.5f) : Bounds.getMin();
            const Elaine::Vector3 Max = Bounds.isNull() ? Elaine::Vector3(0.5f) : Bounds.getMax();
            Elaine::Vector3 LocalMin(FLT_MAX), LocalMax(-FLT_MAX);
            for (int Z = 0; Z < 2; ++Z)
                for (int Y = 0; Y < 2; ++Y)
                    for (int Xc = 0; Xc < 2; ++Xc)
                    {
                        const Elaine::Vector3 Corner(Xc ? Max.x : Min.x, Y ? Max.y : Min.y, Z ? Max.z : Min.z);
                        const Elaine::Vector4 Local = InverseWorld * Elaine::Vector4(Corner.x, Corner.y, Corner.z, 1.0f);
                        LocalMin.x = std::min(LocalMin.x, Local.x);
                        LocalMin.y = std::min(LocalMin.y, Local.y);
                        LocalMin.z = std::min(LocalMin.z, Local.z);
                        LocalMax.x = std::max(LocalMax.x, Local.x);
                        LocalMax.y = std::max(LocalMax.y, Local.y);
                        LocalMax.z = std::max(LocalMax.z, Local.z);
                    }
            LocalBounds[0] = LocalMin.x; LocalBounds[1] = LocalMin.y; LocalBounds[2] = LocalMin.z;
            LocalBounds[3] = LocalMax.x; LocalBounds[4] = LocalMax.y; LocalBounds[5] = LocalMax.z;
            Operation = static_cast<ImGuizmo::OPERATION>(Operation | ImGuizmo::BOUNDS);
        }
        ImGuizmo::Manipulate(ViewData, ProjectionData, Operation, mMode, ModelData, nullptr, nullptr,
            IsVisible(GizmoType::Bounds) ? LocalBounds : nullptr);
        if (!ImGuizmo::IsUsing()) return false;

        Elaine::Matrix4x4 NewWorld(ModelData);
        Elaine::Vector3 Position, Scale;
        Elaine::Quaternion Rotation;
        NewWorld.decomposition(Position, Scale, Rotation);
        if (!std::isfinite(Position.x) || !std::isfinite(Position.y) || !std::isfinite(Position.z) ||
            !std::isfinite(Scale.x) || !std::isfinite(Scale.y) || !std::isfinite(Scale.z))
            return false;

        if (Actor->GetParent())
        {
            Elaine::Matrix4x4 Local = Actor->GetParent()->GetWorldMatrix().inverse() * NewWorld;
            Local.decomposition(Position, Scale, Rotation);
        }
        Actor->SetPosition(Position);
        Actor->SetQuaternion(Rotation);
        Actor->SetScale(Scale);
        return true;
    }
}
