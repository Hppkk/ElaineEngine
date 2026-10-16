#include "ElaineInspectorPanel.h"
#include "ElaineEditorUI.h"
#include "ElainePropertyDrawer.h"
#include "ElainePropertyDrawerRegistry.h"
#include "ElaineTypeDescriptor.h"
#include "imgui.h"
#include "ElaineActor.h"
#include "ElaineComponent.h"
#include "GamePlay/ElaineMeshComponent.h"

namespace Editor
{
	void InspectorPanel::OnDraw()
	{
		Elaine::Actor* selected = mContext->GetSelectedActor();
		if (!selected)
		{
			ImGui::TextDisabled("No Actor selected");
			return;
		}

		// --- Actor Header ---
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.3f, 1.0f));
		ImGui::Text("%s", selected->GetName().c_str());
		ImGui::PopStyleColor();
		ImGui::Separator();

		// --- Name editing ---
		{
			char nameBuf[256];
			strncpy_s(nameBuf, selected->GetName().c_str(), sizeof(nameBuf) - 1);
			if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
			{
				selected->SetName(nameBuf);
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// --- Components ---
		auto& components = selected->GetComponents();
		for (auto& [name, ActorComponent] : components)
		{
			if (!ActorComponent) continue;

			ImGui::PushID(ActorComponent);

			const Elaine::Name& typeName = ActorComponent->GetType();
			std::string typeStr = typeName.ToString();

			// Priority 1: Use generated type-specific drawer (compile-time, precise)
			if (PropertyDrawerRegistry::Instance().HasDrawer(typeStr))
			{
				if (ImGui::CollapsingHeader(typeName.C_Str(), ImGuiTreeNodeFlags_DefaultOpen))
				{
					PropertyDrawerRegistry::Instance().Draw(typeStr, ActorComponent);
				}
			}
			// Priority 2: Fallback to runtime reflection drawer (generic)
			else
			{
				Elaine::TypeDescriptor* desc = Elaine::TypeRegistry::Instance().Find(typeStr);
				if (desc)
				{
					Elaine::PropertyDrawer::DrawComponentInspector(ActorComponent, desc);
				}
				else
				{
					if (ImGui::CollapsingHeader(typeName.C_Str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::TextDisabled("No reflection data available");
					}
				}
			}

			if (auto* MeshComponent = dynamic_cast<Elaine::StaticMeshComponent*>(ActorComponent))
			{
				const auto& Materials = MeshComponent->GetMaterialInstances();
				for (size_t MaterialIndex = 0; MaterialIndex < Materials.size(); ++MaterialIndex)
				{
					if (!Materials[MaterialIndex]) continue;
					ImGui::PushID(static_cast<int>(MaterialIndex));
					if (ImGui::TreeNode("Material Parameters"))
					{
						Elaine::PropertyDrawer::DrawMaterialParameters(Materials[MaterialIndex]);
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}

			ImGui::PopID();
			ImGui::Spacing();
		}

		ImGui::Spacing();
		ImGui::Separator();

		// --- Add ActorComponent button ---
		float width = ImGui::GetContentRegionAvail().x;
		if (ImGui::Button("Add ActorComponent", ImVec2(width, 0)))
		{
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup"))
		{
			// List all registered types that inherit from ActorComponent
			auto& allTypes = Elaine::TypeRegistry::Instance().GetAllTypes();
			for (auto& [typeName, typeDesc] : allTypes)
			{
				const char* parent = typeDesc->GetParentClassName();
				if (parent && (strcmp(parent, "ActorComponent") == 0))
				{
					if (ImGui::MenuItem(typeName.c_str()))
					{
						selected->AddComponent(typeName.c_str());
					}
				}
			}
			ImGui::EndPopup();
		}
	}
}
