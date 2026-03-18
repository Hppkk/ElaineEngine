#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <clang-c/Index.h>

namespace ElaineBuildTool
{
	// ============================================================
	// Metadata map type
	// ============================================================
	using MetaDataMap = std::unordered_map<std::string, std::string>;

	// ============================================================
	// Parsed data structures
	// ============================================================
	struct ParsedProperty
	{
		std::string Name;
		std::string TypeName;
		std::string AccessSpecifier; // "public", "protected", "private"
		MetaDataMap MetaData;        // Parsed from EPROPERTY(...) arguments
	};

	struct ParsedFunctionParam
	{
		std::string Name;
		std::string TypeName;
	};

	struct ParsedFunction
	{
		std::string Name;
		std::string ReturnType;
		std::vector<ParsedFunctionParam> Parameters;
		std::string AccessSpecifier;
		MetaDataMap MetaData;        // Parsed from EFUNCTION(...) arguments
	};

	struct ParsedClass
	{
		std::string ClassName;
		std::string ParentClassName;
		std::string Namespace;
		std::vector<ParsedProperty> Properties;
		std::vector<ParsedFunction> Functions;
		std::string SourceFile;
		bool HasGeneratedBody = false;
		MetaDataMap MetaData;        // Parsed from ECLASS(...) arguments
	};

	// ============================================================
	// HeaderParser — uses libclang to parse headers
	// ============================================================
	class HeaderParser
	{
	public:
		HeaderParser();
		~HeaderParser();

		// Set include paths for parsing
		void AddIncludePath(const std::string& path);

		// Parse a single header file, returns classes found with ECLASS() marker
		bool ParseFile(const std::string& filePath, std::vector<ParsedClass>& outClasses);

		// Parse macro arguments into key=value metadata map
		// e.g. "DisplayName=\"Speed\", Min=0, Max=100, Transient" 
		// => {"DisplayName":"Speed", "Min":"0", "Max":"100", "Transient":""}
		static MetaDataMap ParseMacroArgs(const std::string& argsStr);

	private:
		// Check if a file contains ECLASS marker (quick text scan before full parse)
		bool FileContainsReflectionMarker(const std::string& filePath);

		// AST traversal callback
		static CXChildVisitResult VisitCursor(CXCursor cursor, CXCursor parent, CXClientData clientData);

		// Context passed during traversal
		struct TraversalContext
		{
			std::vector<ParsedClass>* Classes = nullptr;
			ParsedClass* CurrentClass = nullptr;
			std::string CurrentAccessSpecifier = "private";
			std::string CurrentNamespace;
			bool InReflectedClass = false;
			bool NextFieldIsProperty = false;
			bool NextMethodIsFunction = false;
		};

		CXIndex mIndex = nullptr;
		std::vector<std::string> mIncludePaths;
	};
}
