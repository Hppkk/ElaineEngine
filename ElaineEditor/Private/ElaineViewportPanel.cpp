#include "ElaineViewportPanel.h"
#include "imgui.h"
#include "ElaineEditorGlobalContext.h"
#include "ElaineWorld.h"
#include "ElaineViewport.h"
#include "GamePlay/ElaineCameraComponent.h"
#include "GamePlay/ElaineActor.h"
#include "math/ElaineRay.h"
#include "math/ElaineISpatialObject.h"
#include "math/ElaineAxisAlignedBox.h"
#include "imgui/ImGuizmo/ImGuizmo.h"

#include <algorithm>

namespace Editor
{
    namespace
    {
        struct ImageRect { ImVec2 Min; ImVec2 Max; ImVec2 Size() const { return ImVec2(Max.x - Min.x, Max.y - Min.y); } };
        ImageRect FitImage(const ImVec2& Min, const ImVec2& Available, float Aspect)
        {
            ImageRect Result{Min, ImVec2(Min.x + Available.x, Min.y + Available.y)};
            if (Available.x <= 1.0f || Available.y <= 1.0f || Aspect <= 0.0f) return Result;
            if (Available.x / Available.y > Aspect)
            {
                const float Width = Available.y * Aspect;
                Result.Min.x += (Available.x - Width) * 0.5f; Result.Max.x = Result.Min.x + Width;
            }
            else
            {
                const float Height = Available.x / Aspect;
                Result.Min.y += (Available.y - Height) * 0.5f; Result.Max.y = Result.Min.y + Height;
            }
            return Result;
        }
        bool InRect(const ImageRect& Rect, const ImVec2& Point)
        {
            return Point.x >= Rect.Min.x && Point.x <= Rect.Max.x && Point.y >= Rect.Min.y && Point.y <= Rect.Max.y;
        }
    }

    void ViewportPanel::OnDraw()
    {
        auto* Context = EditorGlobalContext::instance();
        auto& Gizmos = Context->GetGizmoManager();
        Gizmos.DrawSettings();
        Context->GetGridManager().SetVisible(Gizmos.IsVisible(GizmoType::Grid));
        const ImVec2 AreaSize = ImGui::GetContentRegionAvail();
        const ImVec2 AreaPos = ImGui::GetCursorScreenPos();

        if (!mViewportSRV || mTexWidth <= 0 || mTexHeight <= 0)
        {
            ImGui::Dummy(AreaSize);
            ImGui::GetWindowDrawList()->AddRectFilled(AreaPos, ImVec2(AreaPos.x + AreaSize.x, AreaPos.y + AreaSize.y), IM_COL32(20, 20, 20, 255));
            return;
        }

        const ImageRect Image = FitImage(AreaPos, AreaSize, static_cast<float>(mTexWidth) / static_cast<float>(mTexHeight));
        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(AreaPos, ImVec2(AreaPos.x + AreaSize.x, AreaPos.y + AreaSize.y), IM_COL32(20, 20, 20, 255));
        ImGui::SetCursorScreenPos(Image.Min);
        ImGui::Image(reinterpret_cast<ImTextureID>(mViewportSRV), Image.Size());
        ImGui::SetCursorScreenPos(AreaPos);
        ImGui::Dummy(AreaSize);

        const bool HoveredImage = InRect(Image, ImGui::GetMousePos());
        const bool RightMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        if (HoveredImage && !ImGui::IsAnyItemActive() && !RightMouseDown)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) { Gizmos.SetOperation(ImGuizmo::TRANSLATE); ImGui::MarkIniSettingsDirty(); }
            if (ImGui::IsKeyPressed(ImGuiKey_E)) { Gizmos.SetOperation(ImGuizmo::ROTATE); ImGui::MarkIniSettingsDirty(); }
            if (ImGui::IsKeyPressed(ImGuiKey_R)) { Gizmos.SetOperation(ImGuizmo::SCALE); ImGui::MarkIniSettingsDirty(); }
            if (ImGui::IsKeyPressed(ImGuiKey_X)) { Gizmos.SetMode(Gizmos.GetMode() == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL); ImGui::MarkIniSettingsDirty(); }
        }

        if (Context->GetSceneViewport() && Context->GetSceneViewport()->GetCamera())
        {
            Elaine::CameraComponent* Camera = Context->GetSceneViewport()->GetCamera();
            mCameraController.Tick(ImGui::GetIO().DeltaTime, HoveredImage, Camera, Context->GetSelectedActor());
            ImGuizmo::SetDrawlist(DrawList);
            ImGuizmo::SetOrthographic(Camera->GetProjectionType() == Elaine::ProjectionType::Orthographic);
            DrawList->PushClipRect(Image.Min, Image.Max, true);

            Elaine::Actor* Selected = Context->GetSelectedActor();
            if (Selected && (Gizmos.IsVisible(GizmoType::Transform) || Gizmos.IsVisible(GizmoType::Bounds)))
                Gizmos.DrawTransform(Selected, Camera->GetViewMatrix(), Camera->GetProjMatrix(), Image.Min.x, Image.Min.y, Image.Size().x, Image.Size().y);

            const bool GizmoInput = ImGuizmo::IsUsing() || ImGuizmo::IsOver();
            if (HoveredImage && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !GizmoInput)
            {
                const ImVec2 Mouse = ImGui::GetMousePos();
                mBoxSelectStartX = Mouse.x; mBoxSelectStartY = Mouse.y; mIsBoxSelecting = false;
            }
            if (HoveredImage && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 5.0f) && !GizmoInput)
            {
                mIsBoxSelecting = true;
                const ImVec2 End = ImGui::GetMousePos();
                DrawList->AddRect(ImVec2(mBoxSelectStartX, mBoxSelectStartY), End, IM_COL32(0, 255, 0, 255));
                DrawList->AddRectFilled(ImVec2(mBoxSelectStartX, mBoxSelectStartY), End, IM_COL32(0, 255, 0, 30));
            }
            if ((HoveredImage || mIsBoxSelecting) && ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !GizmoInput)
            {
                ImVec2 Mouse = ImGui::GetMousePos();
                Mouse.x = std::max(Image.Min.x, std::min(Image.Max.x, Mouse.x));
                Mouse.y = std::max(Image.Min.y, std::min(Image.Max.y, Mouse.y));
                const Elaine::Matrix4x4 View = Camera->GetViewMatrix();
                const Elaine::Matrix4x4 Projection = Camera->GetProjMatrix();
                const Elaine::Matrix4x4 InverseViewProjection = (Projection * View).inverse();
                if (mIsBoxSelecting)
                {
                    const float MinX = std::min(mBoxSelectStartX, Mouse.x);
                    const float MaxX = std::max(mBoxSelectStartX, Mouse.x);
                    const float MinY = std::min(mBoxSelectStartY, Mouse.y);
                    const float MaxY = std::max(mBoxSelectStartY, Mouse.y);
                    const float NdcMinX = ((MinX - Image.Min.x) / Image.Size().x) * 2.0f - 1.0f;
                    const float NdcMaxX = ((MaxX - Image.Min.x) / Image.Size().x) * 2.0f - 1.0f;
                    const float NdcMinY = 1.0f - ((MaxY - Image.Min.y) / Image.Size().y) * 2.0f;
                    const float NdcMaxY = 1.0f - ((MinY - Image.Min.y) / Image.Size().y) * 2.0f;
                    Elaine::AxisAlignedBox Box;
                    Box.setNull();
                    for (int Z = 0; Z < 2; ++Z)
                        for (int Y = 0; Y < 2; ++Y)
                            for (int X = 0; X < 2; ++X)
                            {
                                const float XNdc = X ? NdcMaxX : NdcMinX;
                                const float YNdc = Y ? NdcMaxY : NdcMinY;
                                Elaine::Vector4 Point = InverseViewProjection * Elaine::Vector4(XNdc, YNdc, Z ? 1.0f : 0.0f, 1.0f);
                                if (Point.w != 0.0f)
                                {
                                    Point.x /= Point.w; Point.y /= Point.w; Point.z /= Point.w;
                                    Box.merge(Elaine::Vector3(Point.x, Point.y, Point.z));
                                }
                            }
                    if (Context->GetActiveWorld())
                    {
                        auto Results = Context->GetActiveWorld()->BoxIntersect(Box);
                        if (!Results.empty() && Results.front()->GetUserType() == 1)
                            Context->SetSelectedActor(static_cast<Elaine::Actor*>(Results.front()->GetUserData()));
                    }
                }
                else
                {
                    const float NdcX = ((Mouse.x - Image.Min.x) / Image.Size().x) * 2.0f - 1.0f;
                    const float NdcY = 1.0f - ((Mouse.y - Image.Min.y) / Image.Size().y) * 2.0f;
                    Elaine::Vector4 Target = InverseViewProjection * Elaine::Vector4(NdcX, NdcY, 1.0f, 1.0f);
                    if (Target.w != 0.0f)
                    {
                        Target.x /= Target.w; Target.y /= Target.w; Target.z /= Target.w;
                        Elaine::Vector3 Direction(Target.x - Camera->GetPosition().x, Target.y - Camera->GetPosition().y, Target.z - Camera->GetPosition().z);
                        Direction.normalise();
                        if (Context->GetActiveWorld())
                        {
                            auto Hit = Context->GetActiveWorld()->Raycast(Elaine::Ray(Camera->GetPosition(), Direction));
                            if (Hit && Hit->GetUserType() == 1) Context->SetSelectedActor(static_cast<Elaine::Actor*>(Hit->GetUserData()));
                            else Context->SetSelectedActor(nullptr);
                        }
                    }
                }
                mIsBoxSelecting = false;
            }
            DrawList->PopClipRect();
        }
    }
}
