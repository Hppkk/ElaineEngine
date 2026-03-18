#include "ElaineInspectorPanel.h"
#include "ElaineEditorUI.h"
#include "ElainePropertyDrawer.h"
#include "ElainePropertyDrawerRegistry.h"
#include "ElaineTypeDescriptor.h"
#include "imgui.h"
#include "ElaineGameObject.h"
#include "ElaineComponent.h"

namespace Editor
{
	void InspectorPanel::OnDraw()
	{
		Elaine::GameObject* selected = mContext->GetSelectedGameObject();
		if (!selected)
		{
			ImGui::TextDisabled("No GameObject selected");
			return;
		}

		// --- GameObject Header ---
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
		for (auto& [name, component] : components)
		{
			if (!component) continue;

			ImGui::PushID(component);

			const Elaine::Name& typeName = component->GetType();
			std::string typeStr = typeName.ToString();

			// Priority 1: Use generated type-specific drawer (compile-time, precise)
			if (PropertyDrawerRegistry::Instance().HasDrawer(typeStr))
			{
				if (ImGui::CollapsingHeader(typeName.C_Str(), ImGuiTreeNodeFlags_DefaultOpen))
				{
					PropertyDrawerRegistry::Instance().Draw(typeStr, component);
				}
			}
			// Priority 2: Fallback to runtime reflection drawer (generic)
			else
			{
				Elaine::TypeDescriptor* desc = Elaine::TypeRegistry::Instance().Find(typeStr);
				if (desc)
				{
					Elaine::PropertyDrawer::DrawComponentInspector(component, desc);
				}
				else
				{
					if (ImGui::CollapsingHeader(typeName.C_Str(), ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::TextDisabled("No reflection data available");
					}
				}
			}

			ImGui::PopID();
			ImGui::Spacing();
		}

		ImGui::Spacing();
		ImGui::Separator();

		// --- Add Component button ---
		float width = ImGui::GetContentRegionAvail().x;
		if (ImGui::Button("Add Component", ImVec2(width, 0)))
		{
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup"))
		{
			// List all registered types that inherit from Component
			auto& allTypes = Elaine::TypeRegistry::Instance().GetAllTypes();
			for (auto& [typeName, typeDesc] : allTypes)
			{
				const char* parent = typeDesc->GetParentClassName();
				if (parent && (strcmp(parent, "Component") == 0))
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
