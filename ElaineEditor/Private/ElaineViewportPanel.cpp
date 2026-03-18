#include "ElaineViewportPanel.h"
#include "imgui.h"
#include "ElaineEditorGlobalContext.h"
#include "ElaineWorld.h"
#include "ElaineSceneManager.h"
#include "GamePlay/ElaineCameraComponent.h"
#include "ElaineViewport.h"
#include "imgui/ImGuizmo/ImGuizmo.h"
#include "GamePlay/ElaineGameObject.h"
#include "math/ElaineRay.h"
#include "math/ElaineISpatialObject.h"

namespace Editor
{
	void ViewportPanel::OnDraw()
	{
		ImVec2 panelSize = ImGui::GetContentRegionAvail();
        ImVec2 panelPos = ImGui::GetCursorScreenPos();

		if (mViewportSRV && mTexWidth > 0 && mTexHeight > 0)
		{
			// Shortcuts (Unity-style): W/E/R when viewport hovered and no item active
			if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive())
			{
				if (ImGui::IsKeyPressed(ImGuiKey_W))
					mCurrentGizmoOperation = static_cast<int>(ImGuizmo::TRANSLATE);
				if (ImGui::IsKeyPressed(ImGuiKey_E))
					mCurrentGizmoOperation = static_cast<int>(ImGuizmo::ROTATE);
				if (ImGui::IsKeyPressed(ImGuiKey_R))
					mCurrentGizmoOperation = static_cast<int>(ImGuizmo::SCALE);
				if (ImGui::IsKeyPressed(ImGuiKey_X))
					mCurrentGizmoMode = mCurrentGizmoMode == static_cast<int>(ImGuizmo::LOCAL)
						? static_cast<int>(ImGuizmo::WORLD)
						: static_cast<int>(ImGuizmo::LOCAL);
			}

			// Unity-style Gizmo toolbar above the viewport
			if (ImGui::Button("Move (W)"))
				mCurrentGizmoOperation = static_cast<int>(ImGuizmo::TRANSLATE);
			ImGui::SameLine();
			if (ImGui::Button("Rotate (E)"))
				mCurrentGizmoOperation = static_cast<int>(ImGuizmo::ROTATE);
			ImGui::SameLine();
			if (ImGui::Button("Scale (R)"))
				mCurrentGizmoOperation = static_cast<int>(ImGuizmo::SCALE);
			ImGui::SameLine();
			ImGui::Spacing();
			ImGui::SameLine();
			if (static_cast<ImGuizmo::OPERATION>(mCurrentGizmoOperation) != ImGuizmo::SCALE)
			{
				if (ImGui::RadioButton("Local", mCurrentGizmoMode == static_cast<int>(ImGuizmo::LOCAL)))
					mCurrentGizmoMode = static_cast<int>(ImGuizmo::LOCAL);
				ImGui::SameLine();
				if (ImGui::RadioButton("World", mCurrentGizmoMode == static_cast<int>(ImGuizmo::WORLD)))
					mCurrentGizmoMode = static_cast<int>(ImGuizmo::WORLD);
			}

			// Display the engine's rendered scene (below toolbar)
			ImVec2 imageSize = ImGui::GetContentRegionAvail();
			ImVec2 imagePos = ImGui::GetCursorScreenPos();
			ImGui::Image((ImTextureID)mViewportSRV, imageSize);

            auto* ctx = EditorGlobalContext::instance();
            if (ctx && ctx->GetSceneViewport() && ctx->GetSceneViewport()->GetCamera())
            {
                Elaine::CameraComponent* camComp = ctx->GetSceneViewport()->GetCamera();
                
                ImGuizmo::SetOrthographic(camComp->GetProjectionType() == Elaine::ProjectionType::Orthographic);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(imagePos.x, imagePos.y, imageSize.x, imageSize.y);
                
                Elaine::Matrix4x4 viewMat = camComp->GetViewMatrix();
                Elaine::Matrix4x4 projMat = camComp->GetProjMatrix();
                
                Elaine::GameObject* selectedObj = ctx->GetSelectedGameObject();
                if (selectedObj)
                {
                    Elaine::Matrix4x4 worldMat = selectedObj->GetWorldMatrix();
                    float view[16], proj[16], model[16];
                    viewMat.toData(view);
                    projMat.toData(proj);
                    worldMat.toData(model);
                    
                    ImGuizmo::Manipulate(view, proj,
                        static_cast<ImGuizmo::OPERATION>(mCurrentGizmoOperation),
                        static_cast<ImGuizmo::MODE>(mCurrentGizmoMode),
                        model);
                    
                    if (ImGuizmo::IsUsing())
                    {
                        float pos[3], rot[3], scale[3];
                        ImGuizmo::DecomposeMatrixToComponents(model, pos, rot, scale);
                        selectedObj->SetPosition(Elaine::Vector3(pos[0], pos[1], pos[2]));
                        
                        // Convert euler angles to quaternion
                        Elaine::Matrix4x4 newWorldMat(model);
                        Elaine::Vector3 dPos, dScale;
                        Elaine::Quaternion qRot;
                        newWorldMat.decomposition(dPos, dScale, qRot);
                        selectedObj->SetQuaternion(qRot);
                        selectedObj->SetScale(Elaine::Vector3(scale[0], scale[1], scale[2]));
                    }
                }
                
                // Picking
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver())
                {
                    ImVec2 mousePos = ImGui::GetMousePos();
                    float nx = ((mousePos.x - imagePos.x) / imageSize.x) * 2.0f - 1.0f;
                    float ny = 1.0f - ((mousePos.y - imagePos.y) / imageSize.y) * 2.0f; 
                    
                    Elaine::Matrix4x4 invVP = (projMat * viewMat).inverse();
                    Elaine::Vector4 target = invVP * Elaine::Vector4(nx, ny, 1.0f, 1.0f);
                    if (target.w != 0.0f)
                    {
                        target.x /= target.w;
                        target.y /= target.w;
                        target.z /= target.w;
                    }
                    
                    Elaine::Vector3 dir(target.x - camComp->GetPosition().x, target.y - camComp->GetPosition().y, target.z - camComp->GetPosition().z);
                    dir.normalise();
                    
                    Elaine::Ray ray(camComp->GetPosition(), dir);
                    if (ctx->GetActiveWorld())
                    {
                        auto result = ctx->GetActiveWorld()->Raycast(ray);
                        if (result && result->GetUserType() == 1) // 1 = GameObject
                        {
                            ctx->SetSelectedGameObject(static_cast<Elaine::GameObject*>(result->GetUserData()));
                        }
                        else
                        {
                            ctx->SetSelectedGameObject(nullptr);
                        }
                    }
                }

                // Temporary simple Box Select
                static ImVec2 dragStart(0, 0);
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(1) && !ImGuizmo::IsOver())
                {
                    dragStart = ImGui::GetMousePos();
                }
                if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(1) && !ImGuizmo::IsOver())
                {
                    ImVec2 dragEnd = ImGui::GetMousePos();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    drawList->AddRect(dragStart, dragEnd, IM_COL32(0, 255, 0, 255));
                }
                if (ImGui::IsWindowHovered() && ImGui::IsMouseReleased(1) && !ImGuizmo::IsOver())
                {
                    ImVec2 dragEnd = ImGui::GetMousePos();
                    float minX = std::min(dragStart.x, dragEnd.x);
                    float maxX = std::max(dragStart.x, dragEnd.x);
                    float minY = std::min(dragStart.y, dragEnd.y);
                    float maxY = std::max(dragStart.y, dragEnd.y);

                    // Compute NDC bounds
                    float ndcMinX = ((minX - imagePos.x) / imageSize.x) * 2.0f - 1.0f;
                    float ndcMaxX = ((maxX - imagePos.x) / imageSize.x) * 2.0f - 1.0f;
                    float ndcMinY = 1.0f - ((maxY - imagePos.y) / imageSize.y) * 2.0f; // Y up
                    float ndcMaxY = 1.0f - ((minY - imagePos.y) / imageSize.y) * 2.0f; // Y up

                    // Box Intersect requires a 3D AABB or Frustum against the BVH.
                    // Currently, World::BoxIntersect expects an AxisAlignedBox. 
                    // For a proper 2D drag-box selection, we need to construct a Frustum from the NDC and test it.
                    // To keep it simple for this step (AABB intersect), we construct an AABB from the Frustum corners:
                    
                    Elaine::Matrix4x4 invViewProj = (projMat * viewMat).inverse();
                    Elaine::Vector3 corners[8];
                    int idx = 0;
                    for (int z = 0; z < 2; ++z) {
                        for (int y = 0; y < 2; ++y) {
                            for (int x = 0; x < 2; ++x) {
                                float pX = x ? ndcMaxX : ndcMinX;
                                float pY = y ? ndcMaxY : ndcMinY;
                                float pZ = z ? 1.0f : 0.0f; // Near and Far in D3D/NDC
                                Elaine::Vector4 pt = invViewProj * Elaine::Vector4(pX, pY, pZ, 1.0f);
                                if (pt.w != 0.0f) { pt.x /= pt.w; pt.y /= pt.w; pt.z /= pt.w; }
                                corners[idx++] = Elaine::Vector3(pt.x, pt.y, pt.z);
                            }
                        }
                    }

                    Elaine::AxisAlignedBox frustumAABB;
                    frustumAABB.setNull();
                    for (int i = 0; i < 8; ++i) frustumAABB.merge(corners[i]);

                    if (ctx->GetActiveWorld())
                    {
                        auto results = ctx->GetActiveWorld()->BoxIntersect(frustumAABB);
                        if (!results.empty())
                        {
                            // Just select the first one for now
                            if(results[0]->GetUserType() == 1)
                                ctx->SetSelectedGameObject(static_cast<Elaine::GameObject*>(results[0]->GetUserData()));
                        }
                    }
                }
            }
		}
		else
		{
			// Placeholder when no scene texture is available
			ImVec2 center = ImVec2(
				ImGui::GetCursorScreenPos().x + panelSize.x * 0.5f,
				ImGui::GetCursorScreenPos().y + panelSize.y * 0.5f);

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 topLeft = ImGui::GetCursorScreenPos();
			ImVec2 bottomRight = ImVec2(topLeft.x + panelSize.x, topLeft.y + panelSize.y);

			// Dark background
			drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(20, 20, 20, 255));

			// Grid lines
			const float gridStep = 50.0f;
			for (float x = topLeft.x; x < bottomRight.x; x += gridStep)
				drawList->AddLine(ImVec2(x, topLeft.y), ImVec2(x, bottomRight.y), IM_COL32(40, 40, 40, 255));
			for (float y = topLeft.y; y < bottomRight.y; y += gridStep)
				drawList->AddLine(ImVec2(topLeft.x, y), ImVec2(bottomRight.x, y), IM_COL32(40, 40, 40, 255));

			// Center text
			const char* text = "3D Viewport (No Scene Connected)";
			ImVec2 textSize = ImGui::CalcTextSize(text);
			drawList->AddText(
				ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f),
				IM_COL32(100, 100, 100, 255), text);

			// Advance cursor past the placeholder area
			ImGui::Dummy(panelSize);
		}
	}
}
