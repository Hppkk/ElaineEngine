#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include "ElaineHeaderParser.h"

namespace ElaineBuildTool
{
	// ============================================================
	// CodeGenerator — generates reflection and editor code
	//   Engine output:  .generated.h + .generated.cpp
	//   Editor output:  .editor.generated.h + .editor.generated.cpp
	// ============================================================
	class CodeGenerator
	{
	public:
		CodeGenerator() = default;
		~CodeGenerator() = default;

		void SetOutputDirectory(const std::string& dir) { mOutputDir = dir; }
		void SetEditorOutputDirectory(const std::string& dir) { mEditorOutputDir = dir; }

		// Set the full list of reflected class names (classes with ECLASS).
		// Used to decide whether to generate parent Serialize/Deserialize calls.
		void SetReflectedClassNames(const std::unordered_set<std::string>& names) { mReflectedClassNames = names; }

		// Generate .generated.h — GENERATED_BODY() macro with helper declarations
		bool GenerateForClasses(const std::vector<ParsedClass>& classes,
								const std::string& sourceHeaderPath);

		// Generate .generated.cpp — static registration code (compiled once)
		bool GenerateForClassesCpp(const std::vector<ParsedClass>& classes,
									const std::string& sourceHeaderPath);

		// Generate .editor.generated.h — ImGui draw function declarations
		bool GenerateEditorForClasses(const std::vector<ParsedClass>& classes,
									   const std::string& sourceHeaderPath);

		// Generate .editor.generated.cpp — ImGui draw implementations + auto-registration
		bool GenerateEditorForClassesCpp(const std::vector<ParsedClass>& classes,
										   const std::string& sourceHeaderPath);

	private:
		// .generated.h content
		std::string GenerateClassHeader(const ParsedClass& cls);

		// .generated.cpp content
		std::string GenerateClassCpp(const ParsedClass& cls,
									  const std::string& sourceHeaderName);

		// .editor.generated.h content: declaration only
		std::string GenerateEditorDrawDeclaration(const ParsedClass& cls);

		// .editor.generated.cpp content: implementation + registration
		std::string GenerateEditorDrawImplementation(const ParsedClass& cls);

		// Utilities
		std::string MakeRelativeInclude(const std::string& sourceHeaderPath);
		std::string EscapeStringForCpp(const std::string& str);
		bool WriteFileIfChanged(const std::string& filePath,
								 const std::string& content);

		std::string mOutputDir;
		std::string mEditorOutputDir;
		std::unordered_set<std::string> mReflectedClassNames;
	};
}
