#include "ElaineViewportPanel.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "ElaineImGuiLayer.h"
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
    }

    ViewportPanel::~ViewportPanel()
    {
        mGizmoInput.Reset();
        mSceneInput.Reset();
        if (auto* input = Elaine::InputSystem::TryGet(); input && mRegisteredWindow)
            input->RemoveViewport(mRegisteredWindow, 1);
    }

    void ViewportPanel::InitializeInput()
    {
        using namespace Elaine;
        auto* input = InputSystem::instance();
        mSceneDesc.Name = "SceneViewport";
        mSceneDesc.Priority = 100;
        mSceneDesc.Viewport = 1;
        mSceneDesc.Domain = InputDomain::Scene;
        mSceneDesc.Enabled = false;
        mSceneDesc.HitTest = [this](const InputEvent& e) { return ContainsInput(e); };
        mSceneInput = input->RegisterLayer(mSceneDesc, [this, input](const InputEvent& e) {
            if (e.Type == InputEventType::Button && e.Action == EInputAction::Press)
                input->CapturePointer(mSceneInput, e.Window);
            return e.Type == InputEventType::Cancel ? InputReply::Unhandled : InputReply::Handled;
        });
        mGizmoDesc = mSceneDesc;
        mGizmoDesc.Name = "SceneGizmo";
        mGizmoDesc.Priority = 200;
        mGizmoDesc.KeyboardFocused = false;
        mGizmoDesc.HitTest = [this](const InputEvent& e) {
            return mGizmoHit && (e.Type != InputEventType::Button || e.Button == EMouseButton::Left);
        };
        mGizmoInput = input->RegisterLayer(mGizmoDesc, [this, input](const InputEvent& e) {
            if (e.Type == InputEventType::Button && e.Action == EInputAction::Press)
                input->CapturePointer(mGizmoInput, e.Window);
            return InputReply::Handled;
        });
    }

    void ViewportPanel::BeginInputFrame()
    {
        mImageVisible = false;
        mWindowFocused = false;
        mDrawList = nullptr;
        mGizmoHit = false;
    }
    bool ViewportPanel::ContainsInput(const Elaine::InputEvent& e) const
    {
        return mImageVisible && e.Window == mInputWindow && mImageRect.Contains(e.X, e.Y) &&
            ImGuiLayer::IsWindowAtPosition(mImageWindowId, e);
    }
    bool ViewportPanel::IsDragging() const
    {
        auto* input = Elaine::InputSystem::instance();
        return input->HasPointerCapture(mSceneInput) || input->HasPointerCapture(mGizmoInput);
    }
    void ViewportPanel::UpdateInputLayers()
    {
        auto* input = Elaine::InputSystem::instance();
        if (mRegisteredWindow && (!mImageVisible || mRegisteredWindow != mInputWindow))
            input->RemoveViewport(mRegisteredWindow, 1);
        mRegisteredWindow = mImageVisible ? mInputWindow : 0;
        if (mImageVisible)
        {
            input->SetViewport(mInputWindow, 1, Elaine::InputDomain::Scene, mImageRect);
            if (mWindowFocused) input->SetKeyboardFocus(mInputWindow, 1);
        }
        mSceneDesc.Window = mGizmoDesc.Window = mInputWindow;
        mSceneDesc.Enabled = mGizmoDesc.Enabled = mImageVisible;
        mSceneDesc.KeyboardFocused = mWindowFocused || IsDragging();
        input->UpdateLayer(mSceneInput, mSceneDesc);
        input->UpdateLayer(mGizmoInput, mGizmoDesc);
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

        auto* input = Elaine::InputSystem::instance();
        mImageVisible = true;
        mImageRect = {Image.Min.x, Image.Min.y, Image.Max.x, Image.Max.y};
        mDrawList = DrawList;
        mImageWindowId = ImGui::GetCurrentWindow()->ID;
        mWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        mInputWindow = reinterpret_cast<Elaine::InputWindowId>(ImGui::GetWindowViewport()->PlatformHandleRaw);
        const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            mImageRect.Contains(ImGui::GetMousePos().x, ImGui::GetMousePos().y);
        auto& io = ImGui::GetIO();
        const bool modal = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        const auto& gizmoState = input->GetState(mGizmoInput);
        const bool allowGizmo = !modal && !io.KeyAlt && !io.MouseDown[1] &&
            (hovered || input->HasPointerCapture(mGizmoInput)) &&
            (!io.MouseDown[0] || io.MouseClicked[0] || gizmoState.IsMouseDown(Elaine::EMouseButton::Left));
        auto* viewport = Context->GetSceneViewport();
        auto* selected = Context->GetSelectedActor();
        if (viewport && viewport->GetCamera() && selected &&
            (Gizmos.IsVisible(GizmoType::Transform) || Gizmos.IsVisible(GizmoType::Bounds)))
        {
            auto* camera = viewport->GetCamera();
            ImGuizmo::SetDrawlist(DrawList);
            ImGuizmo::SetOrthographic(camera->GetProjectionType() == Elaine::ProjectionType::Orthographic);
            Gizmos.DrawTransform(selected, camera->GetViewMatrix(), camera->GetProjMatrix(),
                Image.Min.x, Image.Min.y, Image.Size().x, Image.Size().y, allowGizmo);
            mGizmoHit = allowGizmo && (ImGuizmo::IsUsing() || ImGuizmo::IsOver());
        }
        else ImGuizmo::Enable(false);
    }

    void ViewportPanel::ProcessInput()
    {
        using namespace Elaine;
        auto* input = InputSystem::instance();
        auto* Context = EditorGlobalContext::instance();
        auto& Gizmos = Context->GetGizmoManager();
        const auto& state = input->GetState(mSceneInput);
        const auto& gizmo = input->GetState(mGizmoInput);
        // A hidden/closed panel must still discard cancelled gestures this frame.
        for (const auto& event : state.Events)
            if (event.Type == InputEventType::Cancel)
            {
                if (event.CancelPointer) mSelectionActive = mIsBoxSelecting = mRightDown = false;
                if (event.CancelKeyboard && (event.Key == EKeyCode::Unknown || event.Key == EKeyCode::LeftAlt || event.Key == EKeyCode::RightAlt)) mAltDown = false;
            }
        Gizmos.CommitTransform(mImageVisible && !gizmo.Cancelled &&
            (gizmo.IsMouseDown(EMouseButton::Left) || gizmo.IsMouseReleased(EMouseButton::Left)));
        if (gizmo.Cancelled) ImGuizmo::Enable(false);
        auto* viewport = Context->GetSceneViewport();
        if (!viewport || !viewport->GetCamera()) return;
        auto* Camera = viewport->GetCamera();
        mCameraController.Tick(ImGui::GetIO().DeltaTime, mImageVisible && !state.Cancelled,
            state, Camera, Context->GetSelectedActor());
        if (!mImageVisible || !mDrawList) return;
        const ImageRect Image{{mImageRect.Left, mImageRect.Top}, {mImageRect.Right, mImageRect.Bottom}};
        if (!state.IsMouseDown(EMouseButton::Right))
        {
            if (state.IsKeyPressed(EKeyCode::W)) { Gizmos.SetOperation(ImGuizmo::TRANSLATE); ImGui::MarkIniSettingsDirty(); }
            if (state.IsKeyPressed(EKeyCode::E)) { Gizmos.SetOperation(ImGuizmo::ROTATE); ImGui::MarkIniSettingsDirty(); }
            if (state.IsKeyPressed(EKeyCode::R)) { Gizmos.SetOperation(ImGuizmo::SCALE); ImGui::MarkIniSettingsDirty(); }
            if (state.IsKeyPressed(EKeyCode::X)) { Gizmos.SetMode(Gizmos.GetMode() == ImGuizmo::LOCAL ? ImGuizmo::WORLD : ImGuizmo::LOCAL); ImGui::MarkIniSettingsDirty(); }
        }
        for (const auto& event : state.Events)
        {
            if (event.Type == InputEventType::Cancel)
            {
                if (event.CancelPointer) mSelectionActive = mIsBoxSelecting = mRightDown = false;
                if (event.CancelKeyboard && (event.Key == EKeyCode::Unknown || event.Key == EKeyCode::LeftAlt || event.Key == EKeyCode::RightAlt)) mAltDown = false;
                continue;
            }
            if (event.Type == InputEventType::Key && (event.Key == EKeyCode::LeftAlt || event.Key == EKeyCode::RightAlt))
                mAltDown = event.Action != EInputAction::Release;
            if (event.Type == InputEventType::Button && event.Button == EMouseButton::Right)
                mRightDown = event.Action != EInputAction::Release;
            if (event.Type == InputEventType::Button && event.Button == EMouseButton::Left && event.Action == EInputAction::Press && !mAltDown && !mRightDown)
            {
                mSelectionActive = true; mIsBoxSelecting = false;
                mBoxSelectStartX = event.X; mBoxSelectStartY = event.Y;
            }
            if (event.Type == InputEventType::Move && mSelectionActive)
            {
                const float dx = event.X - mBoxSelectStartX, dy = event.Y - mBoxSelectStartY;
                mIsBoxSelecting |= dx * dx + dy * dy >= 25.0f;
            }
            if (event.Type == InputEventType::Button && event.Button == EMouseButton::Left && event.Action == EInputAction::Release && mSelectionActive)
            {
                ImVec2 Mouse(event.X, event.Y);
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
                mIsBoxSelecting = mSelectionActive = false;
            }
        }
        if (mIsBoxSelecting && mSelectionActive)
        {
            mDrawList->PushClipRect(Image.Min, Image.Max, true);
            mDrawList->AddRect(ImVec2(mBoxSelectStartX, mBoxSelectStartY), ImVec2(state.MouseX, state.MouseY), IM_COL32(0, 255, 0, 255));
            mDrawList->AddRectFilled(ImVec2(mBoxSelectStartX, mBoxSelectStartY), ImVec2(state.MouseX, state.MouseY), IM_COL32(0, 255, 0, 30));
            mDrawList->PopClipRect();
        }
    }
}
