#include "ElaineHeaderParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace ElaineBuildTool
{
	// ============================================================
	// Helper to convert CXString to std::string
	// ============================================================
	static std::string CXStringToStd(CXString cxStr)
	{
		const char* cstr = clang_getCString(cxStr);
		std::string result = cstr ? cstr : "";
		clang_disposeString(cxStr);
		return result;
	}

	// ============================================================
	// HeaderParser
	// ============================================================
	HeaderParser::HeaderParser()
	{
		mIndex = clang_createIndex(0, 0);
	}

	HeaderParser::~HeaderParser()
	{
		if (mIndex)
			clang_disposeIndex(mIndex);
	}

	void HeaderParser::AddIncludePath(const std::string& path)
	{
		mIncludePaths.push_back(path);
	}

	bool HeaderParser::FileContainsReflectionMarker(const std::string& filePath)
	{
		std::ifstream file(filePath);
		if (!file.is_open())
			return false;

		std::string line;
		while (std::getline(file, line))
		{
			if (line.find("ECLASS") != std::string::npos)
				return true;
		}
		return false;
	}

	// ============================================================
	// ParseMacroArgs — parse "Key=Value, Flag, Key2=\"str\"" into map
	// ============================================================
	MetaDataMap HeaderParser::ParseMacroArgs(const std::string& argsStr)
	{
		MetaDataMap meta;
		if (argsStr.empty())
			return meta;

		// Simple state machine parser
		std::string trimmed = argsStr;
		// Trim whitespace
		while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t'))
			trimmed.erase(trimmed.begin());
		while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t'))
			trimmed.pop_back();

		if (trimmed.empty())
			return meta;

		// Split by commas (respecting quoted strings)
		std::vector<std::string> tokens;
		std::string current;
		bool inQuote = false;
		for (size_t i = 0; i < trimmed.size(); i++)
		{
			char c = trimmed[i];
			if (c == '"')
			{
				inQuote = !inQuote;
				continue; // Skip quote characters
			}
			if (c == ',' && !inQuote)
			{
				tokens.push_back(current);
				current.clear();
				continue;
			}
			current += c;
		}
		if (!current.empty())
			tokens.push_back(current);

		// Parse each token as Key=Value or Flag
		for (auto& token : tokens)
		{
			// Trim
			while (!token.empty() && (token.front() == ' ' || token.front() == '\t'))
				token.erase(token.begin());
			while (!token.empty() && (token.back() == ' ' || token.back() == '\t'))
				token.pop_back();

			if (token.empty())
				continue;

			auto eqPos = token.find('=');
			if (eqPos != std::string::npos)
			{
				std::string key = token.substr(0, eqPos);
				std::string value = token.substr(eqPos + 1);
				// Trim key/value
				while (!key.empty() && key.back() == ' ') key.pop_back();
				while (!value.empty() && value.front() == ' ') value.erase(value.begin());
				// Remove surrounding quotes from value
				if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
					value = value.substr(1, value.size() - 2);
				meta[key] = value;
			}
			else
			{
				// Flag-style: e.g. "Transient", "Hidden", "ReadOnly"
				meta[token] = "";
			}
		}

		return meta;
	}

	// ============================================================
	// Extract macro arguments between parens from a line
	// e.g. "EPROPERTY(DisplayName=\"Speed\", Min=0)" => "DisplayName=\"Speed\", Min=0"
	// ============================================================
	static std::string ExtractMacroArgs(const std::string& line, const char* macroName)
	{
		size_t pos = line.find(macroName);
		if (pos == std::string::npos)
			return "";

		pos += strlen(macroName);
		// Find opening paren
		while (pos < line.size() && line[pos] != '(')
			pos++;
		if (pos >= line.size())
			return "";
		pos++; // skip '('

		// Find matching closing paren
		int depth = 1;
		size_t start = pos;
		while (pos < line.size() && depth > 0)
		{
			if (line[pos] == '(') depth++;
			else if (line[pos] == ')') depth--;
			if (depth > 0) pos++;
		}

		return line.substr(start, pos - start);
	}

	bool HeaderParser::ParseFile(const std::string& filePath, std::vector<ParsedClass>& outClasses)
	{
		// Quick check: skip files without ECLASS
		if (!FileContainsReflectionMarker(filePath))
			return true;

		// Build clang args
		std::vector<const char*> args;
		args.push_back("-x");
		args.push_back("c++");
		args.push_back("-std=c++20");
		args.push_back("-D__ELAINE_BUILD_TOOL__=1");
		args.push_back("-DECLASS(...)=");
		args.push_back("-DEPROPERTY(...)=");
		args.push_back("-DEFUNCTION(...)=");
		args.push_back("-DGENERATED_BODY()=");
		args.push_back("-DELAINEREFLECTION_EXPORTS=");
		args.push_back("-DELAINECORE_DLLEXPORT=");
		args.push_back("-DEAGLEENGINE_EXPORTS=");
		args.push_back("-DElaineExport=");
		args.push_back("-DElaineEngineExport=");
		args.push_back("-DUSE_REFLECTION=0");
		args.push_back("-D_WIN32");
		args.push_back("-D_UNICODE");
		args.push_back("-DUNICODE");
		args.push_back("-w");
		args.push_back("-ferror-limit=0");
		args.push_back("-fms-compatibility");
		args.push_back("-fms-extensions");

		std::vector<std::string> includeArgs;
		for (auto& inc : mIncludePaths)
		{
			includeArgs.push_back("-I" + inc);
		}
		for (auto& inc : includeArgs)
		{
			args.push_back(inc.c_str());
		}

		CXTranslationUnit tu = clang_parseTranslationUnit(
			mIndex,
			filePath.c_str(),
			args.data(),
			static_cast<int>(args.size()),
			nullptr, 0,
			CXTranslationUnit_SkipFunctionBodies |
			CXTranslationUnit_DetailedPreprocessingRecord |
			CXTranslationUnit_KeepGoing
		);

		if (!tu)
		{
			std::cerr << "[ElaineBuildTool] Error: Failed to parse " << filePath << std::endl;
			return false;
		}

		unsigned numDiags = clang_getNumDiagnostics(tu);
		for (unsigned i = 0; i < numDiags; ++i)
		{
			CXDiagnostic diag = clang_getDiagnostic(tu, i);
			CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diag);
			if (severity == CXDiagnostic_Fatal)
			{
				std::string msg = CXStringToStd(clang_formatDiagnostic(diag,
					clang_defaultDiagnosticDisplayOptions()));
				std::cerr << "[ElaineBuildTool] Fatal: " << msg << std::endl;
			}
			clang_disposeDiagnostic(diag);
		}

		// --- Text-based pre-scan with metadata extraction ---
		struct MarkerInfo
		{
			int line;
			std::string type;      // "ECLASS", "EPROPERTY", "EFUNCTION", "GENERATED_BODY"
			MetaDataMap metaData;  // Parsed arguments
		};

		std::vector<MarkerInfo> markers;
		{
			std::ifstream src(filePath);
			std::string line;
			int lineNum = 0;
			while (std::getline(src, line))
			{
				lineNum++;
				auto findMacro = [&](const char* macro, const char* type)
				{
					size_t pos = line.find(macro);
					if (pos != std::string::npos)
					{
						if (pos > 0)
						{
							char before = line[pos - 1];
							if (std::isalnum(before) || before == '_')
								return;
						}
						// Extract and parse macro arguments
						std::string argsStr = ExtractMacroArgs(line, type);
						MetaDataMap meta = ParseMacroArgs(argsStr);
						markers.push_back({ lineNum, type, meta });
					}
				};
				findMacro("ECLASS(", "ECLASS");
				findMacro("EPROPERTY(", "EPROPERTY");
				findMacro("EFUNCTION(", "EFUNCTION");
				findMacro("GENERATED_BODY(", "GENERATED_BODY");
			}
		}

		if (markers.empty())
		{
			clang_disposeTranslationUnit(tu);
			return true;
		}

		// Build line-indexed lookups with metadata
		struct MarkerWithMeta
		{
			int line;
			MetaDataMap meta;
			bool consumed = false;
		};
		std::vector<MarkerWithMeta> eclassMarkers, epropertyMarkers, efunctionMarkers;
		for (auto& m : markers)
		{
			if (m.type == "ECLASS") eclassMarkers.push_back({ m.line, m.metaData });
			else if (m.type == "EPROPERTY") epropertyMarkers.push_back({ m.line, m.metaData });
			else if (m.type == "EFUNCTION") efunctionMarkers.push_back({ m.line, m.metaData });
		}

		// --- AST Traversal ---
		TraversalContext ctx;
		ctx.Classes = &outClasses;

		struct FullContext
		{
			TraversalContext* tc = nullptr;
			std::vector<MarkerWithMeta>* eclassMarkers;
			std::vector<MarkerWithMeta>* epropertyMarkers;
			std::vector<MarkerWithMeta>* efunctionMarkers;
			std::string filePath;
		};

		FullContext fullCtx;
		fullCtx.tc = &ctx;
		fullCtx.eclassMarkers = &eclassMarkers;
		fullCtx.epropertyMarkers = &epropertyMarkers;
		fullCtx.efunctionMarkers = &efunctionMarkers;
		fullCtx.filePath = filePath;

		struct Visitor
		{
			static CXChildVisitResult Visit(CXCursor cursor, CXCursor parent, CXClientData data)
			{
				auto* fc = static_cast<FullContext*>(data);
				auto* ctx = fc->tc;

				CXCursorKind kind = clang_getCursorKind(cursor);

				CXSourceLocation loc = clang_getCursorLocation(cursor);
				CXFile file;
				unsigned line, column, offset;
				clang_getSpellingLocation(loc, &file, &line, &column, &offset);

				std::string cursorFile = CXStringToStd(clang_getFileName(file));

				if (cursorFile.empty())
					return CXChildVisit_Continue;

				std::string targetFile = fc->filePath;
				std::replace(targetFile.begin(), targetFile.end(), '\\', '/');
				std::replace(cursorFile.begin(), cursorFile.end(), '\\', '/');

				if (cursorFile.find(targetFile) == std::string::npos &&
					targetFile.find(cursorFile) == std::string::npos)
				{
					auto getFilename = [](const std::string& p) -> std::string
					{
						auto pos = p.find_last_of('/');
						return (pos != std::string::npos) ? p.substr(pos + 1) : p;
					};
					if (getFilename(cursorFile) != getFilename(targetFile))
						return CXChildVisit_Continue;
				}

				if (kind == CXCursor_Namespace)
				{
					std::string nsName = CXStringToStd(clang_getCursorSpelling(cursor));
					std::string prevNs = ctx->CurrentNamespace;
					ctx->CurrentNamespace = prevNs.empty() ? nsName : (prevNs + "::" + nsName);
					clang_visitChildren(cursor, Visitor::Visit, data);
					ctx->CurrentNamespace = prevNs;
					return CXChildVisit_Continue;
				}

				if (kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl)
				{
					std::string className = CXStringToStd(clang_getCursorSpelling(cursor));
					
					if (className.empty() || !clang_isCursorDefinition(cursor))
						return CXChildVisit_Continue;

					// Check if this class is near an ECLASS marker, and capture metadata
					bool isReflected = false;
					MetaDataMap classMeta;
					for (auto& em : *fc->eclassMarkers)
					{
						if (line >= (unsigned)em.line && line <= (unsigned)(em.line + 3))
						{
							isReflected = true;
							classMeta = em.meta;
							break;
						}
					}

					if (isReflected)
					{
						ParsedClass cls;
						cls.ClassName = className;
						cls.Namespace = ctx->CurrentNamespace;
						cls.SourceFile = fc->filePath;
						cls.MetaData = classMeta;

						// Get parent class
						clang_visitChildren(cursor,
							[](CXCursor c, CXCursor p, CXClientData d) -> CXChildVisitResult
							{
								if (clang_getCursorKind(c) == CXCursor_CXXBaseSpecifier)
								{
									CXType baseType = clang_getCursorType(c);
									std::string baseName = CXStringToStd(clang_getTypeSpelling(baseType));
									auto colonPos = baseName.rfind("::");
									if (colonPos != std::string::npos)
										baseName = baseName.substr(colonPos + 2);
									static_cast<ParsedClass*>(d)->ParentClassName = baseName;
								}
								return CXChildVisit_Continue;
							},
							&cls);

						auto prevClass = ctx->CurrentClass;
						bool prevInReflected = ctx->InReflectedClass;
						ctx->CurrentClass = &cls;
						ctx->InReflectedClass = true;
						ctx->CurrentAccessSpecifier = (kind == CXCursor_StructDecl) ? "public" : "private";

						clang_visitChildren(cursor, Visitor::VisitClassMember, data);

						ctx->Classes->push_back(cls);
						ctx->CurrentClass = prevClass;
						ctx->InReflectedClass = prevInReflected;
						return CXChildVisit_Continue;
					}

					return CXChildVisit_Recurse;
				}

				return CXChildVisit_Recurse;
			}

			static CXChildVisitResult VisitClassMember(CXCursor cursor, CXCursor parent, CXClientData data)
			{
				auto* fc = static_cast<FullContext*>(data);
				auto* ctx = fc->tc;

				if (!ctx->CurrentClass)
					return CXChildVisit_Continue;

				CXCursorKind kind = clang_getCursorKind(cursor);

				CXSourceLocation loc = clang_getCursorLocation(cursor);
				CXFile file;
				unsigned line, column, offset;
				clang_getSpellingLocation(loc, &file, &line, &column, &offset);

				if (kind == CXCursor_CXXAccessSpecifier)
				{
					CX_CXXAccessSpecifier access = clang_getCXXAccessSpecifier(cursor);
					switch (access)
					{
					case CX_CXXPublic:    ctx->CurrentAccessSpecifier = "public"; break;
					case CX_CXXProtected: ctx->CurrentAccessSpecifier = "protected"; break;
					case CX_CXXPrivate:   ctx->CurrentAccessSpecifier = "private"; break;
					default: break;
					}
					return CXChildVisit_Continue;
				}

				// Field declaration — check if preceded by EPROPERTY with metadata
				if (kind == CXCursor_FieldDecl)
				{
					MetaDataMap propMeta;
					bool isProperty = false;
					for (auto& pm : *fc->epropertyMarkers)
					{
						if (!pm.consumed && line >= (unsigned)pm.line && line <= (unsigned)(pm.line + 2))
						{
							isProperty = true;
							propMeta = pm.meta;
							pm.consumed = true;
							break;
						}
					}

					if (isProperty)
					{
						ParsedProperty prop;
						prop.Name = CXStringToStd(clang_getCursorSpelling(cursor));
						CXType fieldType = clang_getCursorType(cursor);
						prop.TypeName = CXStringToStd(clang_getTypeSpelling(fieldType));
						prop.AccessSpecifier = ctx->CurrentAccessSpecifier;
						prop.MetaData = propMeta;

						auto colonPos = prop.TypeName.rfind("::");
						if (colonPos != std::string::npos)
						{
							std::string shortName = prop.TypeName.substr(colonPos + 2);
							if (prop.TypeName.find("Elaine::") != std::string::npos)
								prop.TypeName = shortName;
						}

						ctx->CurrentClass->Properties.push_back(prop);
					}
					return CXChildVisit_Continue;
				}

				// Method declaration — check if preceded by EFUNCTION with metadata
				if (kind == CXCursor_CXXMethod)
				{
					MetaDataMap funcMeta;
					bool isFunction = false;
					for (auto& fm : *fc->efunctionMarkers)
					{
						if (!fm.consumed && line >= (unsigned)fm.line && line <= (unsigned)(fm.line + 2))
						{
							isFunction = true;
							funcMeta = fm.meta;
							fm.consumed = true;
							break;
						}
					}

					if (isFunction)
					{
						ParsedFunction func;
						func.Name = CXStringToStd(clang_getCursorSpelling(cursor));
						CXType resultType = clang_getResultType(clang_getCursorType(cursor));
						func.ReturnType = CXStringToStd(clang_getTypeSpelling(resultType));
						func.AccessSpecifier = ctx->CurrentAccessSpecifier;
						func.MetaData = funcMeta;

						int numArgs = clang_Cursor_getNumArguments(cursor);
						for (int i = 0; i < numArgs; i++)
						{
							CXCursor argCursor = clang_Cursor_getArgument(cursor, i);
							ParsedFunctionParam param;
							param.Name = CXStringToStd(clang_getCursorSpelling(argCursor));
							param.TypeName = CXStringToStd(
								clang_getTypeSpelling(clang_getCursorType(argCursor)));
							func.Parameters.push_back(param);
						}

						ctx->CurrentClass->Functions.push_back(func);
					}
					return CXChildVisit_Continue;
				}

				return CXChildVisit_Continue;
			}
		};

		CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
		clang_visitChildren(rootCursor, Visitor::Visit, &fullCtx);

		clang_disposeTranslationUnit(tu);

		std::cout << "[ElaineBuildTool] Parsed " << filePath 
				  << " — found " << outClasses.size() << " reflected class(es)" << std::endl;

		return true;
	}
}
