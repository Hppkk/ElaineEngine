#pragma once
#include "ElaineTypeDescriptor.h"
#include "imgui.h"
#include <string>
#include <cstring>
#include <unordered_map>
#include <functional>

namespace Elaine
{
	// ============================================================
	// IPropertyWidget — interface for custom type ImGui widgets
	// ============================================================
	class IPropertyWidget
	{
	public:
		virtual ~IPropertyWidget() = default;
		// Draw the widget and return true if value changed
		virtual bool Draw(const char* label, void* data, const PropertyDescriptor& prop) = 0;
	};

	// ============================================================
	// PropertyWidgetRegistry — register custom widgets per type
	// ============================================================
	class PropertyWidgetRegistry
	{
	public:
		static PropertyWidgetRegistry& Instance()
		{
			static PropertyWidgetRegistry sInstance;
			return sInstance;
		}

		void Register(const std::string& typeName, IPropertyWidget* widget)
		{
			mWidgets[typeName] = widget;
		}

		IPropertyWidget* Find(const std::string& typeName) const
		{
			auto it = mWidgets.find(typeName);
			return (it != mWidgets.end()) ? it->second : nullptr;
		}

	private:
		PropertyWidgetRegistry() = default;
		std::unordered_map<std::string, IPropertyWidget*> mWidgets;
	};

	// ============================================================
	// PropertyDrawer — runtime reflection-driven ImGui property panel
	// ============================================================
	class PropertyDrawer
	{
	public:
		// Draw all visible properties (flat layout)
		static bool DrawProperties(void* obj, const TypeDescriptor* desc);

		// Draw properties grouped by Category with CollapsingHeaders
		static bool DrawPropertiesGrouped(void* obj, const TypeDescriptor* desc);

		// Draw a single property widget
		static bool DrawProperty(void* obj, const PropertyDescriptor& prop);

		// Draw a full component inspector (type title + grouped properties)
		static bool DrawComponentInspector(void* obj, const TypeDescriptor* desc);

	private:
		// Built-in type widget dispatch
		static bool DrawFloat(const char* label, void* data, const PropertyDescriptor& prop);
		static bool DrawInt(const char* label, void* data, const PropertyDescriptor& prop);
		static bool DrawBool(const char* label, void* data, const PropertyDescriptor& prop);
		static bool DrawString(const char* label, void* data, const PropertyDescriptor& prop);
		static bool DrawUint8(const char* label, void* data, const PropertyDescriptor& prop);
	};
}
