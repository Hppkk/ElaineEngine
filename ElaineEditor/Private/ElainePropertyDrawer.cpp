#include "ElainePropertyDrawer.h"
#include <vector>
#include <algorithm>

namespace Elaine
{
	// ============================================================
	// DrawFloat
	// ============================================================
	bool PropertyDrawer::DrawFloat(const char* label, void* data, const PropertyDescriptor& prop)
	{
		float* val = static_cast<float*>(data);
		bool hasMin = prop.HasMeta("Min");
		bool hasMax = prop.HasMeta("Max");
		float speed = prop.GetMetaFloat("Speed", 0.1f);

		if (hasMin && hasMax)
		{
			float minV = prop.GetMetaFloat("Min", 0.f);
			float maxV = prop.GetMetaFloat("Max", 100.f);
			return ImGui::SliderFloat(label, val, minV, maxV);
		}
		else
		{
			return ImGui::DragFloat(label, val, speed);
		}
	}

	// ============================================================
	// DrawInt
	// ============================================================
	bool PropertyDrawer::DrawInt(const char* label, void* data, const PropertyDescriptor& prop)
	{
		int* val = static_cast<int*>(data);
		bool hasMin = prop.HasMeta("Min");
		bool hasMax = prop.HasMeta("Max");

		if (hasMin && hasMax)
		{
			int minV = prop.GetMetaInt("Min", 0);
			int maxV = prop.GetMetaInt("Max", 100);
			return ImGui::SliderInt(label, val, minV, maxV);
		}
		else
		{
			return ImGui::DragInt(label, val);
		}
	}

	// ============================================================
	// DrawBool
	// ============================================================
	bool PropertyDrawer::DrawBool(const char* label, void* data, const PropertyDescriptor& prop)
	{
		return ImGui::Checkbox(label, static_cast<bool*>(data));
	}

	// ============================================================
	// DrawString
	// ============================================================
	bool PropertyDrawer::DrawString(const char* label, void* data, const PropertyDescriptor& prop)
	{
		auto* str = static_cast<std::string*>(data);
		char buf[512];
		strncpy_s(buf, str->c_str(), sizeof(buf) - 1);

		bool multiline = prop.HasMeta("Multiline");
		bool changed = false;

		if (multiline)
		{
			changed = ImGui::InputTextMultiline(label, buf, sizeof(buf));
		}
		else
		{
			changed = ImGui::InputText(label, buf, sizeof(buf));
		}

		if (changed)
			*str = buf;

		return changed;
	}

	// ============================================================
	// DrawUint8
	// ============================================================
	bool PropertyDrawer::DrawUint8(const char* label, void* data, const PropertyDescriptor& prop)
	{
		int v = *static_cast<unsigned char*>(data);
		bool changed = ImGui::DragInt(label, &v, 1.0f, 0, 255);
		if (changed)
			*static_cast<unsigned char*>(data) = static_cast<unsigned char>(v);
		return changed;
	}

	// ============================================================
	// DrawProperty — single property dispatch
	// ============================================================
	bool PropertyDrawer::DrawProperty(void* obj, const PropertyDescriptor& prop)
	{
		if (prop.IsHidden())
			return false;

		void* data = prop.GetPtrIn(obj);
		const char* label = prop.GetDisplayName();
		const char* typeName = prop.TypeName;

		bool readOnly = prop.IsReadOnly();
		if (readOnly)
			ImGui::BeginDisabled();

		// Check for custom widget first
		IPropertyWidget* custom = PropertyWidgetRegistry::Instance().Find(typeName);
		bool changed = false;

		if (custom)
		{
			changed = custom->Draw(label, data, prop);
		}
		else if (strcmp(typeName, "float") == 0)
		{
			changed = DrawFloat(label, data, prop);
		}
		else if (strcmp(typeName, "double") == 0)
		{
			float v = static_cast<float>(*static_cast<double*>(data));
			if (ImGui::DragFloat(label, &v, 0.1f))
			{
				*static_cast<double*>(data) = v;
				changed = true;
			}
		}
		else if (strcmp(typeName, "int") == 0 || strcmp(typeName, "int32") == 0 ||
				 strcmp(typeName, "int32_t") == 0)
		{
			changed = DrawInt(label, data, prop);
		}
		else if (strcmp(typeName, "uint32") == 0 || strcmp(typeName, "uint32_t") == 0 ||
				 strcmp(typeName, "unsigned int") == 0)
		{
			changed = DrawInt(label, data, prop);
		}
		else if (strcmp(typeName, "uint8") == 0 || strcmp(typeName, "uint8_t") == 0 ||
				 strcmp(typeName, "unsigned char") == 0)
		{
			changed = DrawUint8(label, data, prop);
		}
		else if (strcmp(typeName, "bool") == 0)
		{
			changed = DrawBool(label, data, prop);
		}
		else if (strcmp(typeName, "std::string") == 0 ||
				 strcmp(typeName, "std::basic_string<char>") == 0)
		{
			changed = DrawString(label, data, prop);
		}
		else if (strcmp(typeName, "Vector3") == 0)
		{
			changed = ImGui::DragFloat3(label, static_cast<float*>(data), 0.1f);
		}
		else if (strcmp(typeName, "Quaternion") == 0)
		{
			changed = ImGui::DragFloat4(label, static_cast<float*>(data), 0.01f);
		}
		else
		{
			// Check if this is a nested reflected type
			TypeDescriptor* nested = TypeRegistry::Instance().Find(typeName);
			if (nested)
			{
				if (ImGui::TreeNode(label))
				{
					changed = DrawProperties(data, nested);
					ImGui::TreePop();
				}
			}
			else
			{
				ImGui::LabelText(label, "[%s]", typeName);
			}
		}

		// Tooltip
		const char* tooltip = prop.GetTooltip();
		if (tooltip && tooltip[0] != '\0')
		{
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
				ImGui::SetTooltip("%s", tooltip);
		}

		if (readOnly)
			ImGui::EndDisabled();

		return changed;
	}

	// ============================================================
	// DrawProperties — flat layout
	// ============================================================
	bool PropertyDrawer::DrawProperties(void* obj, const TypeDescriptor* desc)
	{
		if (!obj || !desc)
			return false;

		bool changed = false;
		for (auto& prop : desc->GetProperties())
		{
			changed |= DrawProperty(obj, prop);
		}
		return changed;
	}

	// ============================================================
	// DrawPropertiesGrouped — grouped by Category
	// ============================================================
	bool PropertyDrawer::DrawPropertiesGrouped(void* obj, const TypeDescriptor* desc)
	{
		if (!obj || !desc)
			return false;

		bool changed = false;
		auto categories = desc->GetCategories();

		if (categories.size() <= 1)
		{
			// Single category — just draw flat
			return DrawProperties(obj, desc);
		}

		for (auto& cat : categories)
		{
			auto props = desc->GetPropertiesInCategory(cat.c_str());
			if (props.empty())
				continue;

			if (ImGui::CollapsingHeader(cat.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (auto* prop : props)
				{
					changed |= DrawProperty(obj, *prop);
				}
			}
		}

		return changed;
	}

	// ============================================================
	// DrawComponentInspector — full component panel
	// ============================================================
	bool PropertyDrawer::DrawComponentInspector(void* obj, const TypeDescriptor* desc)
	{
		if (!obj || !desc)
			return false;

		const char* className = desc->GetClassName();

		ImGui::PushID(className);
		
		bool headerOpen = ImGui::CollapsingHeader(className, ImGuiTreeNodeFlags_DefaultOpen);
		
		if (headerOpen)
		{
			ImGui::Indent(10.f);
			bool changed = DrawPropertiesGrouped(obj, desc);
			ImGui::Unindent(10.f);
			ImGui::PopID();
			return changed;
		}

		ImGui::PopID();
		return false;
	}
}
