#include "ElainePropertyDrawer.h"
#include <vector>
#include <algorithm>
#include "math/ElaineTransform.h"
#include "ElaineCameraState.h"

namespace Elaine
{
	bool PropertyDrawer::DrawMaterialParameters(MaterialInstanceDynamic* material)
	{
		if (!material || !material->IsValid()) return false;
		bool changed = false;
		for (const auto& param : material->GetParameterLayout())
		{
			ImGui::PushID(static_cast<int>(param.Hash));
			switch (param.Kind)
			{
			case MaterialParameterKind::Scalar:
			{
				float value = material->GetScalar(Name(param.Name.c_str()), param.DefaultValue.x);
				bool edit = param.HasRange ? ImGui::SliderFloat(param.Name.c_str(), &value, param.Min, param.Max) : ImGui::DragFloat(param.Name.c_str(), &value, 0.01f);
				if (edit) { material->SetScalar(Name(param.Name.c_str()), value); changed = true; }
				break;
			}
			case MaterialParameterKind::Vector:
			case MaterialParameterKind::Color:
			{
				Vector4 value = material->GetVector(Name(param.Name.c_str()), param.DefaultValue);
				bool edit = param.Kind == MaterialParameterKind::Color ? ImGui::ColorEdit4(param.Name.c_str(), value.ptr()) : ImGui::DragFloat4(param.Name.c_str(), value.ptr(), 0.01f);
				if (edit) { material->SetVector(Name(param.Name.c_str()), value); changed = true; }
				break;
			}
			case MaterialParameterKind::Integer:
			{
				int value = material->GetInt(Name(param.Name.c_str()), static_cast<int>(param.DefaultValue.x));
				if (ImGui::DragInt(param.Name.c_str(), &value)) { material->SetInt(Name(param.Name.c_str()), value); changed = true; }
				break;
			}
			case MaterialParameterKind::Boolean:
			{
				bool value = material->GetBool(Name(param.Name.c_str()), param.DefaultValue.x != 0.0f);
				if (ImGui::Checkbox(param.Name.c_str(), &value)) { material->SetBool(Name(param.Name.c_str()), value); changed = true; }
				break;
			}
			default: ImGui::TextDisabled("%s (unsupported)", param.Name.c_str()); break;
			}
			ImGui::PopID();
		}
		return changed;
	}
    namespace
    {
        struct EditResult
        {
            bool Changed = false, Finished = false, Active = false;
            std::string Path;
        };

        EditResult DrawValue(const char* Label, void* Value, const PropertyDescriptor& Property, const std::string& Path)
        {
            EditResult Result;
            Result.Path = Path;
            const std::string Type = Property.TypeName ? Property.TypeName : "";
            auto* Custom = PropertyWidgetRegistry::Instance().Find(Type);
            if (Custom) Result.Changed = Custom->Draw(Label, Value, Property);
            else if (Type == "float")
            {
                auto* Number = static_cast<float*>(Value);
                Result.Changed = Property.HasMeta("Min") && Property.HasMeta("Max") ?
                    ImGui::SliderFloat(Label, Number, Property.GetMetaFloat("Min"), Property.GetMetaFloat("Max")) :
                    ImGui::DragFloat(Label, Number, Property.GetMetaFloat("Speed", 0.1f));
            }
            else if (Type == "double") Result.Changed = ImGui::InputDouble(Label, static_cast<double*>(Value));
            else if (Type == "int" || Type == "int32" || Type == "int32_t")
                Result.Changed = ImGui::DragInt(Label, static_cast<int*>(Value), 1.0f,
                    Property.GetMetaInt("Min", 0), Property.GetMetaInt("Max", 0));
            else if (Type == "uint32" || Type == "uint32_t" || Type == "unsigned int")
                Result.Changed = ImGui::InputScalar(Label, ImGuiDataType_U32, Value);
            else if (Type == "uint8" || Type == "uint8_t" || Type == "unsigned char")
            {
                int Number = *static_cast<unsigned char*>(Value);
                Result.Changed = ImGui::DragInt(Label, &Number, 1.0f, 0, 255, "%d", ImGuiSliderFlags_AlwaysClamp);
                if (Result.Changed) *static_cast<unsigned char*>(Value) = static_cast<unsigned char>(Number);
            }
            else if (Type == "bool") Result.Changed = ImGui::Checkbox(Label, static_cast<bool*>(Value));
            else if (Type == "ProjectionType")
            {
                int Index = static_cast<int>(*static_cast<ProjectionType*>(Value));
                Result.Changed = ImGui::Combo(Label, &Index, "Perspective\0Orthographic\0");
                if (Result.Changed) *static_cast<ProjectionType*>(Value) = static_cast<ProjectionType>(Index);
            }
            else if (Type == "std::string" || Type == "std::basic_string<char>")
            {
                auto& String = *static_cast<std::string*>(Value);
                // Resize callback keeps arbitrarily long strings editable without truncation.
                if (String.capacity() < 64) String.reserve(64);
                auto Resize = [](ImGuiInputTextCallbackData* Data) -> int {
                    auto& Text = *static_cast<std::string*>(Data->UserData);
                    Text.resize(static_cast<size_t>(Data->BufTextLen)); Data->Buf = Text.data(); return 0;
                };
                const auto Flags = ImGuiInputTextFlags_CallbackResize;
                Result.Changed = Property.HasMeta("Multiline") ?
                    ImGui::InputTextMultiline(Label, String.data(), String.capacity() + 1, ImVec2(0, 0), Flags, Resize, &String) :
                    ImGui::InputText(Label, String.data(), String.capacity() + 1, Flags, Resize, &String);
                if (Result.Changed) String.resize(std::strlen(String.c_str()));
            }
            else if (Type == "Vector3") Result.Changed = ImGui::DragFloat3(Label, static_cast<Vector3*>(Value)->ptr(), 0.1f);
            else if (Type == "Quaternion") Result.Changed = ImGui::DragFloat4(Label, static_cast<Quaternion*>(Value)->ptr(), 0.01f);
            else if (Type == "Transform")
            {
                if (ImGui::TreeNode(Label))
                {
                    auto& TransformValue = *static_cast<Transform*>(Value);
                    auto Member = [&](const char* MemberLabel, auto& Field, const char* MemberName, const char* TypeName) {
                        PropertyDescriptor Child; Child.TypeName = TypeName;
                        const auto Edit = DrawValue(MemberLabel, &Field, Child, Path + "." + MemberName);
                        if (Edit.Changed || Edit.Finished) Result = Edit;
                    };
                    Member("Position", TransformValue.mPosition, "mPosition", "Vector3");
                    Member("Rotation (wxyz)", TransformValue.mRotation, "mRotation", "Quaternion");
                    Member("Scale", TransformValue.mScale, "mScale", "Vector3");
                    if (Result.Changed) TransformValue.mDirty = true;
                    ImGui::TreePop();
                }
                return Result;
            }
            else if (const auto* Nested = TypeRegistry::Instance().Find(Type))
            {
                if (ImGui::TreeNode(Label))
                {
                    for (const auto& Child : Nested->GetProperties())
                    {
                        if (Child.IsHidden()) continue;
                        PropertyEditValue ChildCopy(Child, Value);
                        if (!ChildCopy.Get()) continue;
                        ImGui::PushID(Child.Name);
                        ImGui::BeginDisabled(Child.IsReadOnly());
                        const auto Edit = DrawValue(Child.GetDisplayName(), ChildCopy.Get(), Child, Path + "." + Child.Name);
                        ImGui::EndDisabled();
                        ImGui::PopID();
                        if (!Child.IsReadOnly())
                        {
                            if (Edit.Changed && Child.AssignValue) Child.AssignValue(Child.GetPtrIn(Value), ChildCopy.Get());
                            if (Edit.Changed || Edit.Finished)
                            {
                                Result.Path = Edit.Path;
                                Result.Changed |= Edit.Changed; Result.Finished |= Edit.Finished; Result.Active |= Edit.Active;
                            }
                        }
                    }
                    ImGui::TreePop();
                }
                return Result;
            }
            else ImGui::LabelText(Label, "[%s]", Type.c_str());
            Result.Finished = ImGui::IsItemDeactivatedAfterEdit();
            Result.Active = ImGui::IsItemActive();
            return Result;
        }

        struct CollectedProperty
        {
            const PropertyDescriptor* Property;
            const TypeDescriptor* DeclaringType;
        };

        void CollectProperties(const TypeDescriptor* Descriptor, std::vector<CollectedProperty>& Properties)
        {
            const char* Parent = Descriptor->GetParentClassName();
            if (Parent)
                if (auto* Base = TypeRegistry::Instance().Find(Parent)) CollectProperties(Base, Properties);
            for (const auto& Property : Descriptor->GetProperties()) Properties.push_back({&Property, Descriptor});
        }
    }

    bool PropertyDrawer::DrawProperty(void* Object, const PropertyDescriptor& Property, const TypeDescriptor* Owner)
    {
        if (!Object || !Owner || Property.IsHidden()) return false;
        PropertyEditValue Copy(Property, Object);
        if (!Copy.Get()) { ImGui::TextDisabled("%s (no editor value adapter)", Property.GetDisplayName()); return false; }
        ImGui::PushID(Property.Name);
        ImGui::BeginDisabled(Property.IsReadOnly());
        const auto Edit = DrawValue(Property.GetDisplayName(), Copy.Get(), Property, Property.Name);
        ImGui::EndDisabled();
        if (Property.GetTooltip()[0] && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
            ImGui::SetTooltip("%s", Property.GetTooltip());
        ImGui::PopID();
        if (Property.IsReadOnly()) return false;
        bool Changed = false;
        if (Edit.Changed)
            Changed = Owner->CommitPropertyEdit(Object, Property, Copy.Get(), Edit.Path,
                Edit.Active ? PropertyChangeType::Interactive : PropertyChangeType::ValueSet);
        if (Edit.Finished) Owner->NotifyPropertyChanged(Object, {&Property, Edit.Path, PropertyChangeType::EditFinished});
        return Changed;
    }

    bool PropertyDrawer::DrawProperties(void* Object, const TypeDescriptor* Descriptor)
    {
        if (!Object || !Descriptor) return false;
        std::vector<CollectedProperty> Properties;
        CollectProperties(Descriptor, Properties);
        bool Changed = false;
        for (const auto& Entry : Properties) Changed |= DrawProperty(Object, *Entry.Property, Descriptor);
        return Changed;
    }

    bool PropertyDrawer::DrawPropertiesGrouped(void* Object, const TypeDescriptor* Descriptor)
    {
        if (!Object || !Descriptor) return false;
        std::vector<CollectedProperty> Properties;
        CollectProperties(Descriptor, Properties);
        // Inherited class-name categories are implementation details, not inspector sections.
        auto CategoryOf = [Descriptor](const CollectedProperty& Entry) -> std::string {
            const std::string Category = Entry.Property->GetCategory();
            return Entry.DeclaringType != Descriptor && Category == Entry.DeclaringType->GetClassName() ? "" : Category;
        };
        std::vector<std::string> Categories;
        for (const auto& Entry : Properties)
        {
            const auto Category = CategoryOf(Entry);
            if (!Entry.Property->IsHidden() && std::find(Categories.begin(), Categories.end(), Category) == Categories.end())
                Categories.push_back(Category);
        }
        bool Changed = false;
        for (const auto& Category : Categories)
            if (Category.empty() || Categories.size() == 1 || ImGui::CollapsingHeader(Category.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                for (const auto& Entry : Properties)
                    if (Category == CategoryOf(Entry)) Changed |= DrawProperty(Object, *Entry.Property, Descriptor);
        return Changed;
    }

    bool PropertyDrawer::DrawComponentInspector(void* Object, const TypeDescriptor* Descriptor)
    {
        if (!Object || !Descriptor) return false;
        ImGui::PushID(Object);
        const bool Changed = ImGui::CollapsingHeader(Descriptor->GetClassName(), ImGuiTreeNodeFlags_DefaultOpen) &&
            DrawPropertiesGrouped(Object, Descriptor);
        ImGui::PopID();
        return Changed;
    }
}
