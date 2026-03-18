#include "ElaineCodeGenerator.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace ElaineBuildTool
{
	// ============================================================
	// Utilities
	// ============================================================
	std::string CodeGenerator::MakeRelativeInclude(const std::string& sourceHeaderPath)
	{
		fs::path p(sourceHeaderPath);
		return p.filename().string();
	}

	std::string CodeGenerator::EscapeStringForCpp(const std::string& str)
	{
		std::string result;
		result.reserve(str.size() + 10);
		for (char c : str)
		{
			switch (c)
			{
			case '\\': result += "\\\\"; break;
			case '"':  result += "\\\""; break;
			case '\n': result += "\\n"; break;
			case '\t': result += "\\t"; break;
			default:   result += c; break;
			}
		}
		return result;
	}

	bool CodeGenerator::WriteFileIfChanged(const std::string& filePath,
		const std::string& content)
	{
		if (fs::exists(filePath))
		{
			std::ifstream existing(filePath);
			std::stringstream buf;
			buf << existing.rdbuf();
			if (buf.str() == content)
			{
				std::cout << "[ElaineBuildTool] Skipped (unchanged): "
					<< fs::path(filePath).filename().string() << std::endl;
				return true;
			}
		}

		std::ofstream outFile(filePath);
		if (!outFile.is_open())
		{
			std::cerr << "[ElaineBuildTool] Error: Cannot write to " << filePath << std::endl;
			return false;
		}

		outFile << content;
		outFile.close();
		std::cout << "[ElaineBuildTool] Generated: "
			<< fs::path(filePath).filename().string() << std::endl;
		return true;
	}

	// ============================================================
	// GenerateClassHeader — .generated.h content per class
	//   Outputs: #undef/#define GENERATED_BODY() with helpers
	// ============================================================
	std::string CodeGenerator::GenerateClassHeader(const ParsedClass& cls)
	{
		std::ostringstream ss;

		std::string fullClassName = cls.ClassName;
		if (!cls.Namespace.empty())
			fullClassName = cls.Namespace + "::" + cls.ClassName;

		std::string ns = cls.Namespace.empty() ? "Elaine" : cls.Namespace;

		ss << "// ======================================================\n";
		ss << "// GENERATED_BODY for " << fullClassName << "\n";
		ss << "// ======================================================\n\n";

		// Forward declare the registration struct
		ss << "namespace " << ns << " { namespace Generated { struct Z_Register_" << cls.ClassName << "; } }\n";
		// Editor drawer forward-declare (only when editor is enabled)
		ss << "#ifdef _HAS_EDITOR_\n";
		ss << "namespace " << ns << " { namespace EditorGenerated { struct Z_Drawer_" << cls.ClassName << "; } }\n";
		ss << "#endif\n\n";

		// Undef previous GENERATED_BODY and redefine
		// Two versions: with and without editor friend
		ss << "#undef GENERATED_BODY\n";
		ss << "#ifdef _HAS_EDITOR_\n";
		ss << "#define GENERATED_BODY() \\\n";
		ss << "    friend struct " << ns << "::Generated::Z_Register_" << cls.ClassName << "; \\\n";
		ss << "    friend struct " << ns << "::EditorGenerated::Z_Drawer_" << cls.ClassName << "; \\\n";
		ss << "public: \\\n";
		ss << "    static Elaine::TypeDescriptor* StaticClass(); \\\n";
		ss << "    virtual Elaine::TypeDescriptor* GetClass() const; \\\n";
		ss << "    static const char* StaticClassName() { return \"" << cls.ClassName << "\"; } \\\n";
		ss << "    virtual void Serialize(nlohmann::json& outJson) const; \\\n";
		ss << "    virtual void Deserialize(const nlohmann::json& inJson); \\\n";
		ss << "private:\n";
		ss << "#else\n";
		ss << "#define GENERATED_BODY() \\\n";
		ss << "    friend struct " << ns << "::Generated::Z_Register_" << cls.ClassName << "; \\\n";
		ss << "public: \\\n";
		ss << "    static Elaine::TypeDescriptor* StaticClass(); \\\n";
		ss << "    virtual Elaine::TypeDescriptor* GetClass() const; \\\n";
		ss << "    static const char* StaticClassName() { return \"" << cls.ClassName << "\"; } \\\n";
		ss << "    virtual void Serialize(nlohmann::json& outJson) const; \\\n";
		ss << "    virtual void Deserialize(const nlohmann::json& inJson); \\\n";
		ss << "private:\n";
		ss << "#endif\n\n";

		return ss.str();
	}

	// ============================================================
	// GenerateClassCpp — .generated.cpp content per class
	//   Outputs: StaticClass/GetClass impl + static registration
	// ============================================================
	std::string CodeGenerator::GenerateClassCpp(const ParsedClass& cls,
		const std::string& sourceHeaderName)
	{
		std::ostringstream ss;

		std::string fullClassName = cls.ClassName;
		if (!cls.Namespace.empty())
			fullClassName = cls.Namespace + "::" + cls.ClassName;

		std::string ns = cls.Namespace.empty() ? "Elaine" : cls.Namespace;

		ss << "// ======================================================\n";
		ss << "// Static registration for " << fullClassName << "\n";
		ss << "// ======================================================\n\n";

		// --- StaticClass / GetClass implementation ---
		ss << "Elaine::TypeDescriptor* " << fullClassName << "::StaticClass()\n";
		ss << "{\n";
		ss << "    return Elaine::TypeRegistry::Instance().Find(\"" << cls.ClassName << "\");\n";
		ss << "}\n\n";

		ss << "Elaine::TypeDescriptor* " << fullClassName << "::GetClass() const\n";
		ss << "{\n";
		ss << "    return StaticClass();\n";
		ss << "}\n\n";

		// --- Serialize implementation ---
		ss << "void " << fullClassName << "::Serialize(nlohmann::json& outJson) const\n";
		ss << "{\n";
		if (!cls.ParentClassName.empty() && mReflectedClassNames.count(cls.ParentClassName))
		{
			std::string fullParent = cls.ParentClassName;
			if (!cls.Namespace.empty())
				fullParent = cls.Namespace + "::" + cls.ParentClassName;
			ss << "    " << fullParent << "::Serialize(outJson);\n";
		}
		ss << "    Elaine::ReflectionSerializer::Serialize(this, StaticClass(), outJson);\n";
		ss << "}\n\n";

		// --- Deserialize implementation ---
		ss << "void " << fullClassName << "::Deserialize(const nlohmann::json& inJson)\n";
		ss << "{\n";
		if (!cls.ParentClassName.empty() && mReflectedClassNames.count(cls.ParentClassName))
		{
			std::string fullParent = cls.ParentClassName;
			if (!cls.Namespace.empty())
				fullParent = cls.Namespace + "::" + cls.ParentClassName;
			ss << "    " << fullParent << "::Deserialize(inJson);\n";
		}
		ss << "    Elaine::ReflectionSerializer::Deserialize(this, StaticClass(), inJson);\n";
		ss << "}\n\n";

		// --- Registration struct (friend of the class) ---
		ss << "namespace " << ns << " {\n";
		ss << "namespace Generated {\n\n";

		ss << "struct Z_Register_" << cls.ClassName << "\n";
		ss << "{\n";
		ss << "    static Elaine::TypeDescriptor* Register()\n";
		ss << "    {\n";
		ss << "        static Elaine::TypeDescriptor desc;\n";
		ss << "        desc.SetClassName(\"" << cls.ClassName << "\");\n";

		if (!cls.ParentClassName.empty())
			ss << "        desc.SetParentClassName(\"" << cls.ParentClassName << "\");\n";
		else
			ss << "        desc.SetParentClassName(nullptr);\n";

		// Class metadata
		for (auto& [key, value] : cls.MetaData)
		{
			ss << "        desc.SetMeta(\"" << EscapeStringForCpp(key) << "\", \""
			   << EscapeStringForCpp(value) << "\");\n";
		}

		ss << "\n";

		// Properties with metadata
		if (!cls.Properties.empty())
		{
			ss << "        // Properties\n";
			for (auto& prop : cls.Properties)
			{
				ss << "        {\n";
				ss << "            Elaine::PropertyDescriptor pd;\n";
				ss << "            pd.Name = \"" << prop.Name << "\";\n";
				ss << "            pd.TypeName = \"" << prop.TypeName << "\";\n";
				ss << "            pd.Offset = offsetof(" << fullClassName << ", " << prop.Name << ");\n";
				ss << "            pd.Size = sizeof(((" << fullClassName << "*)nullptr)->" << prop.Name << ");\n";
				for (auto& [key, value] : prop.MetaData)
				{
					ss << "            pd.Meta[\"" << EscapeStringForCpp(key) << "\"] = \""
					   << EscapeStringForCpp(value) << "\";\n";
				}
				ss << "            desc.AddProperty(pd);\n";
				ss << "        }\n";
			}
			ss << "\n";
		}

		// Functions with metadata
		if (!cls.Functions.empty())
		{
			ss << "        // Functions\n";
			for (auto& func : cls.Functions)
			{
				ss << "        {\n";
				ss << "            Elaine::FunctionDescriptor fd;\n";
				ss << "            fd.Name = \"" << func.Name << "\";\n";
				ss << "            fd.ReturnTypeName = \"" << func.ReturnType << "\";\n";
				for (auto& param : func.Parameters)
				{
					ss << "            fd.Parameters.push_back({\"" << param.Name << "\", \""
					   << param.TypeName << "\"});\n";
				}
				for (auto& [key, value] : func.MetaData)
				{
					ss << "            fd.Meta[\"" << EscapeStringForCpp(key) << "\"] = \""
					   << EscapeStringForCpp(value) << "\";\n";
				}
				ss << "            desc.AddFunction(fd);\n";
				ss << "        }\n";
			}
			ss << "\n";
		}

		ss << "        Elaine::TypeRegistry::Instance().Register(&desc);\n";
		ss << "        return &desc;\n";
		ss << "    }\n";
		ss << "}; // struct Z_Register_" << cls.ClassName << "\n\n";

		// Static variable triggers registration at startup
		ss << "static Elaine::TypeDescriptor* s_" << cls.ClassName
		   << "_TypeDesc = Z_Register_" << cls.ClassName << "::Register();\n\n";

		ss << "} // namespace Generated\n";
		ss << "} // namespace " << ns << "\n";

		return ss.str();
	}

	// ============================================================
	// GenerateForClasses — produces .generated.h
	// ============================================================
	bool CodeGenerator::GenerateForClasses(const std::vector<ParsedClass>& classes,
		const std::string& sourceHeaderPath)
	{
		if (classes.empty())
			return true;

		fs::create_directories(mOutputDir);

		fs::path srcPath(sourceHeaderPath);
		std::string baseName = srcPath.stem().string();
		std::string generatedFileName = baseName + ".generated.h";
		fs::path outputPath = fs::path(mOutputDir) / generatedFileName;

		std::ostringstream content;
		content << "// ============================================================\n";
		content << "// AUTO-GENERATED FILE by ElaineBuildTool - DO NOT EDIT\n";
		content << "// Source: " << srcPath.filename().string() << "\n";
		content << "// ============================================================\n";
		content << "#pragma once\n\n";
		content << "#include \"ElaineTypeDescriptor.h\"\n";
		content << "#include <nlohmann/json_fwd.hpp>\n\n";

		for (auto& cls : classes)
		{
			content << GenerateClassHeader(cls);
		}

		return WriteFileIfChanged(outputPath.string(), content.str());
	}

	// ============================================================
	// GenerateForClassesCpp — produces .generated.cpp
	// ============================================================
	bool CodeGenerator::GenerateForClassesCpp(const std::vector<ParsedClass>& classes,
		const std::string& sourceHeaderPath)
	{
		if (classes.empty())
			return true;

		fs::create_directories(mOutputDir);

		fs::path srcPath(sourceHeaderPath);
		std::string baseName = srcPath.stem().string();
		std::string generatedCppName = baseName + ".generated.cpp";
		fs::path outputPath = fs::path(mOutputDir) / generatedCppName;

		std::string sourceHeaderName = srcPath.filename().string();

		std::ostringstream content;
		content << "// ============================================================\n";
		content << "// AUTO-GENERATED FILE by ElaineBuildTool - DO NOT EDIT\n";
		content << "// Source: " << sourceHeaderName << "\n";
		content << "// ============================================================\n\n";
		content << "#include \"" << sourceHeaderName << "\"\n";
		content << "#include \"ElaineTypeDescriptor.h\"\n";
		content << "#include \"ElaineSerializer.h\"\n";
		content << "#include <nlohmann/json.hpp>\n";
		content << "#include <cstddef> // offsetof\n\n";

		for (auto& cls : classes)
		{
			content << GenerateClassCpp(cls, sourceHeaderName);
			content << "\n";
		}

		return WriteFileIfChanged(outputPath.string(), content.str());
	}

	// ============================================================
	// Editor Code Generation — split into .h (declaration) + .cpp (impl + registration)
	// ============================================================

	std::string CodeGenerator::GenerateEditorDrawDeclaration(const ParsedClass& cls)
	{
		std::ostringstream ss;
		std::string fullClassName = cls.ClassName;
		if (!cls.Namespace.empty())
			fullClassName = cls.Namespace + "::" + cls.ClassName;

		ss << "// Draw properties for " << fullClassName << "\n";
		ss << "struct Z_Drawer_" << cls.ClassName << "\n";
		ss << "{\n";
		ss << "    static bool Draw(" << fullClassName << "* obj);\n";
		ss << "};\n\n";
		return ss.str();
	}

	std::string CodeGenerator::GenerateEditorDrawImplementation(const ParsedClass& cls)
	{
		std::ostringstream ss;

		std::string fullClassName = cls.ClassName;
		if (!cls.Namespace.empty())
			fullClassName = cls.Namespace + "::" + cls.ClassName;

		ss << "// Draw properties for " << fullClassName << "\n";
		ss << "bool Z_Drawer_" << cls.ClassName << "::Draw(" << fullClassName << "* obj)\n";
		ss << "{\n";
		ss << "    bool changed = false;\n";

		if (cls.Properties.empty())
		{
			ss << "    ImGui::TextDisabled(\"No reflected properties\");\n";
			ss << "    return changed;\n";
			ss << "}\n";
			return ss.str();
		}

		// Group by category
		std::unordered_map<std::string, std::vector<const ParsedProperty*>> categories;
		for (auto& prop : cls.Properties)
		{
			if (prop.MetaData.count("Hidden"))
				continue;

			std::string cat = "Default";
			auto catIt = prop.MetaData.find("Category");
			if (catIt != prop.MetaData.end() && !catIt->second.empty())
				cat = catIt->second;
			categories[cat].push_back(&prop);
		}

		// Maintain stable category order
		std::vector<std::string> catOrder;
		for (auto& prop : cls.Properties)
		{
			if (prop.MetaData.count("Hidden")) continue;
			std::string cat = "Default";
			auto catIt = prop.MetaData.find("Category");
			if (catIt != prop.MetaData.end() && !catIt->second.empty())
				cat = catIt->second;
			bool found = false;
			for (auto& c : catOrder) { if (c == cat) { found = true; break; } }
			if (!found) catOrder.push_back(cat);
		}

		for (auto& cat : catOrder)
		{
			auto& props = categories[cat];
			if (props.empty()) continue;

			bool needsHeader = (catOrder.size() > 1);
			if (needsHeader)
				ss << "    if (ImGui::CollapsingHeader(\"" << EscapeStringForCpp(cat) << "\", ImGuiTreeNodeFlags_DefaultOpen))\n    {\n";

			for (auto* prop : props)
			{
				std::string indent = needsHeader ? "        " : "    ";

				std::string displayName = prop->Name;
				auto dnIt = prop->MetaData.find("DisplayName");
				if (dnIt != prop->MetaData.end())
					displayName = dnIt->second;

				bool readOnly = prop->MetaData.count("ReadOnly") > 0;

				if (readOnly)
					ss << indent << "ImGui::BeginDisabled();\n";

				if (prop->TypeName == "float")
				{
					bool hasRange = prop->MetaData.count("Min") && prop->MetaData.count("Max");
					if (hasRange)
					{
						std::string minVal = prop->MetaData.at("Min");
						std::string maxVal = prop->MetaData.at("Max");
						ss << indent << "changed |= ImGui::SliderFloat(\""
						   << EscapeStringForCpp(displayName) << "\", &obj->"
						   << prop->Name << ", " << minVal << "f, " << maxVal << "f);\n";
					}
					else
					{
						ss << indent << "changed |= ImGui::DragFloat(\""
						   << EscapeStringForCpp(displayName) << "\", &obj->"
						   << prop->Name << ", 0.1f);\n";
					}
				}
				else if (prop->TypeName == "int" || prop->TypeName == "int32" ||
						 prop->TypeName == "int32_t" || prop->TypeName == "uint32" ||
						 prop->TypeName == "uint32_t")
				{
					bool hasRange = prop->MetaData.count("Min") && prop->MetaData.count("Max");
					if (hasRange)
					{
						std::string minVal = prop->MetaData.at("Min");
						std::string maxVal = prop->MetaData.at("Max");
						ss << indent << "changed |= ImGui::SliderInt(\""
						   << EscapeStringForCpp(displayName) << "\", (int*)&obj->"
						   << prop->Name << ", " << minVal << ", " << maxVal << ");\n";
					}
					else
					{
						ss << indent << "changed |= ImGui::DragInt(\""
						   << EscapeStringForCpp(displayName) << "\", (int*)&obj->"
						   << prop->Name << ");\n";
					}
				}
				else if (prop->TypeName == "uint8" || prop->TypeName == "uint8_t")
				{
					ss << indent << "{ int v = obj->" << prop->Name << "; "
					   << "if (ImGui::DragInt(\"" << EscapeStringForCpp(displayName)
					   << "\", &v, 1.0f, 0, 255)) { obj->" << prop->Name
					   << " = (uint8)v; changed = true; } }\n";
				}
				else if (prop->TypeName == "bool")
				{
					ss << indent << "changed |= ImGui::Checkbox(\""
					   << EscapeStringForCpp(displayName) << "\", &obj->"
					   << prop->Name << ");\n";
				}
				else if (prop->TypeName == "std::string" ||
						 prop->TypeName == "std::basic_string<char>")
				{
					ss << indent << "{\n";
					ss << indent << "    char buf[256];\n";
					ss << indent << "    strncpy_s(buf, obj->" << prop->Name
					   << ".c_str(), sizeof(buf) - 1);\n";
					ss << indent << "    if (ImGui::InputText(\"" << EscapeStringForCpp(displayName)
					   << "\", buf, sizeof(buf))) { obj->" << prop->Name
					   << " = buf; changed = true; }\n";
					ss << indent << "}\n";
				}
				else if (prop->TypeName == "Vector3")
				{
					ss << indent << "changed |= ImGui::DragFloat3(\""
					   << EscapeStringForCpp(displayName) << "\", (float*)&obj->"
					   << prop->Name << ", 0.1f);\n";
				}
				else if (prop->TypeName == "Quaternion")
				{
					ss << indent << "changed |= ImGui::DragFloat4(\""
					   << EscapeStringForCpp(displayName) << "\", (float*)&obj->"
					   << prop->Name << ", 0.01f);\n";
				}
				else
				{
					ss << indent << "ImGui::LabelText(\""
					   << EscapeStringForCpp(displayName) << "\", \""
					   << prop->TypeName << "\");\n";
				}

				// Tooltip
				auto tipIt = prop->MetaData.find("Tooltip");
				if (tipIt != prop->MetaData.end() && !tipIt->second.empty())
				{
					ss << indent << "if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))\n";
					ss << indent << "    ImGui::SetTooltip(\""
					   << EscapeStringForCpp(tipIt->second) << "\");\n";
				}

				if (readOnly)
					ss << indent << "ImGui::EndDisabled();\n";
			}

			if (needsHeader)
				ss << "    }\n";
		}

		ss << "    return changed;\n";
		ss << "}\n";

		return ss.str();
	}

	// ============================================================
	// GenerateEditorForClasses — produces .editor.generated.h (declarations)
	// ============================================================
	bool CodeGenerator::GenerateEditorForClasses(const std::vector<ParsedClass>& classes,
		const std::string& sourceHeaderPath)
	{
		if (classes.empty())
			return true;

		std::string outDir = mEditorOutputDir.empty() ? mOutputDir : mEditorOutputDir;
		fs::create_directories(outDir);

		fs::path srcPath(sourceHeaderPath);
		std::string baseName = srcPath.stem().string();
		std::string editorFileName = baseName + ".editor.generated.h";
		fs::path outputPath = fs::path(outDir) / editorFileName;

		std::ostringstream content;
		content << "// ============================================================\n";
		content << "// AUTO-GENERATED EDITOR FILE by ElaineBuildTool - DO NOT EDIT\n";
		content << "// Source: " << srcPath.filename().string() << "\n";
		content << "// ============================================================\n";
		content << "#pragma once\n\n";

		// Forward declarations only — no imgui include needed in header
		content << "#include \"" << srcPath.filename().string() << "\"\n\n";

		std::string nsName = "Elaine";
		if (!classes.empty() && !classes[0].Namespace.empty())
			nsName = classes[0].Namespace;

		content << "namespace " << nsName << " {\n";
		content << "namespace EditorGenerated {\n\n";

		for (auto& cls : classes)
		{
			content << GenerateEditorDrawDeclaration(cls);
		}

		content << "} // namespace EditorGenerated\n";
		content << "} // namespace " << nsName << "\n";

		return WriteFileIfChanged(outputPath.string(), content.str());
	}

	// ============================================================
	// GenerateEditorForClassesCpp — produces .editor.generated.cpp
	//   (implementations + auto-registration into PropertyDrawerRegistry)
	// ============================================================
	bool CodeGenerator::GenerateEditorForClassesCpp(const std::vector<ParsedClass>& classes,
		const std::string& sourceHeaderPath)
	{
		if (classes.empty())
			return true;

		std::string outDir = mEditorOutputDir.empty() ? mOutputDir : mEditorOutputDir;
		fs::create_directories(outDir);

		fs::path srcPath(sourceHeaderPath);
		std::string baseName = srcPath.stem().string();
		std::string editorCppName = baseName + ".editor.generated.cpp";
		fs::path outputPath = fs::path(outDir) / editorCppName;

		std::string sourceHeaderName = srcPath.filename().string();

		std::string nsName = "Elaine";
		if (!classes.empty() && !classes[0].Namespace.empty())
			nsName = classes[0].Namespace;

		std::ostringstream content;
		content << "// ============================================================\n";
		content << "// AUTO-GENERATED EDITOR FILE by ElaineBuildTool - DO NOT EDIT\n";
		content << "// Source: " << sourceHeaderName << "\n";
		content << "// ============================================================\n\n";
		content << "#include \"" << sourceHeaderName << "\"\n";
		content << "#include \"" << baseName << ".editor.generated.h\"\n";
		content << "#include \"imgui.h\"\n";
		content << "#include \"ElainePropertyDrawerRegistry.h\"\n";
		content << "#include <cstring>\n\n";

		content << "namespace " << nsName << " {\n";
		content << "namespace EditorGenerated {\n\n";

		for (auto& cls : classes)
		{
			content << GenerateEditorDrawImplementation(cls);
			content << "\n";

			// Auto-registration into PropertyDrawerRegistry
			std::string fullClassName = cls.ClassName;
			if (!cls.Namespace.empty())
				fullClassName = cls.Namespace + "::" + cls.ClassName;

			content << "static Editor::AutoDrawerRegister s_Drawer_" << cls.ClassName
					<< "(\"" << cls.ClassName << "\", [](void* obj) -> bool {\n";
			content << "    return Z_Drawer_" << cls.ClassName << "::Draw(static_cast<"
					<< fullClassName << "*>(obj));\n";
			content << "});\n\n";
		}

		content << "} // namespace EditorGenerated\n";
		content << "} // namespace " << nsName << "\n";

		return WriteFileIfChanged(outputPath.string(), content.str());
	}
}

