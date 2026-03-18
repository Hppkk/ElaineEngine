#include "ElaineSceneHierarchyPanel.h"
#include "ElaineEditorUI.h"
#include "imgui.h"
#include "ElaineWorld.h"
#include "ElaineGameObject.h"

namespace Editor
{
	void SceneHierarchyPanel::DrawGameObjectNode(Elaine::GameObject* obj)
	{
		if (!obj) return;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth;

		bool isSelected = (mContext->GetSelectedGameObject() == obj);
		if (isSelected)
			flags |= ImGuiTreeNodeFlags_Selected;

		// Check if has children - if not, make it a leaf
		auto& children = obj->GetComponents(); // Use as indicator for now
		// TODO: check actual children GameObjects

		bool opened = ImGui::TreeNodeEx(
			(void*)(intptr_t)obj,
			flags,
			"%s", obj->GetName().c_str());

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			mContext->SetSelectedGameObject(obj);
		}

		// Right-click context menu
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Add Child GameObject"))
			{
				obj->CreateChildGameObject();
			}
			if (ImGui::MenuItem("Delete"))
			{
				obj->Destroy();
				if (isSelected)
					mContext->SetSelectedGameObject(nullptr);
			}
			ImGui::EndPopup();
		}

		if (opened)
		{
			// TODO: iterate child GameObjects when API is available
			ImGui::TreePop();
		}
	}

	void SceneHierarchyPanel::OnDraw()
	{
		Elaine::World* world = mContext->GetActiveWorld();
		if (!world)
		{
			ImGui::TextDisabled("No active world");
			return;
		}

		// Toolbar
		if (ImGui::Button("+ Add GameObject"))
		{
			world->CreateGameObject();
		}
		ImGui::Separator();

		// Draw all root GameObjects
		auto& gameObjects = world->GetGameObjects();
		for (auto* obj : gameObjects)
		{
			if (obj && !obj->GetParent()) // Only root objects
			{
				DrawGameObjectNode(obj);
			}
		}
	}
}
