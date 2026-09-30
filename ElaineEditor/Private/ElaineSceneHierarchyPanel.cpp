#include "ElaineSceneHierarchyPanel.h"
#include "ElaineEditorUI.h"
#include "imgui.h"
#include "ElaineWorld.h"
#include "ElaineActor.h"

namespace Editor
{
	void SceneHierarchyPanel::DrawActorNode(Elaine::Actor* obj)
	{
		if (!obj) return;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_SpanAvailWidth;

		bool isSelected = (mContext->GetSelectedActor() == obj);
		if (isSelected)
			flags |= ImGuiTreeNodeFlags_Selected;

		// Check if has children - if not, make it a leaf
		auto& children = obj->GetComponents(); // Use as indicator for now
		// TODO: check actual children Actors

		bool opened = ImGui::TreeNodeEx(
			(void*)(intptr_t)obj,
			flags,
			"%s", obj->GetName().c_str());

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			mContext->SetSelectedActor(obj);
		}

		// Right-click context menu
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Add Child Actor"))
			{
				obj->CreateChildActor();
			}
			if (ImGui::MenuItem("Delete"))
			{
				obj->Destroy();
				if (isSelected)
					mContext->SetSelectedActor(nullptr);
			}
			ImGui::EndPopup();
		}

		if (opened)
		{
			// TODO: iterate child Actors when API is available
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
		if (ImGui::Button("+ Add Actor"))
		{
			world->CreateActor();
		}
		ImGui::Separator();

		// Draw all root Actors
		auto& Actors = world->GetActors();
		for (auto* obj : Actors)
		{
			if (obj && !obj->GetParent()) // Only root objects
			{
				DrawActorNode(obj);
			}
		}
	}
}
