#include "ElaineHeaderParser.h"
#include "ElaineCodeGenerator.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>

namespace fs = std::filesystem;

// ============================================================
// Usage: ElaineBuildTool.exe --input <dir> --output <dir> [--include <dir>]...
// ============================================================

struct CommandLineArgs
{
	std::vector<std::string> InputDirs;
	std::string OutputDir;
	std::string EditorOutputDir;
	std::vector<std::string> IncludeDirs;
};

bool ParseCommandLine(int argc, char* argv[], CommandLineArgs& args)
{
	for (int i = 1; i < argc; i++)
	{
		std::string arg = argv[i];
		if ((arg == "--input" || arg == "-i") && i + 1 < argc)
		{
			args.InputDirs.push_back(argv[++i]);
		}
		else if ((arg == "--output" || arg == "-o") && i + 1 < argc)
		{
			args.OutputDir = argv[++i];
		}
		else if ((arg == "--editor-output" || arg == "-eo") && i + 1 < argc)
		{
			args.EditorOutputDir = argv[++i];
		}
		else if ((arg == "--include" || arg == "-I") && i + 1 < argc)
		{
			args.IncludeDirs.push_back(argv[++i]);
		}
		else if (arg == "--help" || arg == "-h")
		{
			std::cout << "ElaineBuildTool — Reflection Code Generator\n"
					  << "Usage: ElaineBuildTool --input <dir> --output <dir> [--include <dir>]...\n"
					  << "\n"
					  << "Options:\n"
					  << "  --input, -i     Directory containing headers to scan (can specify multiple)\n"
					  << "  --output, -o    Output directory for .generated.h files\n"
					  << "  --include, -I   Additional include directory (can specify multiple)\n"
					  << "  --help, -h      Show this help\n";
			return false;
		}
	}

	if (args.InputDirs.empty() || args.OutputDir.empty())
	{
		std::cerr << "[ElaineBuildTool] Error: --input and --output are required.\n";
		std::cerr << "Run with --help for usage.\n";
		return false;
	}

	return true;
}

void CollectHeaders(const std::string& dir, std::vector<std::string>& headers)
{
	if (!fs::exists(dir))
	{
		std::cerr << "[ElaineBuildTool] Warning: Input directory does not exist: " << dir << std::endl;
		return;
	}

	for (auto& entry : fs::recursive_directory_iterator(dir))
	{
		if (entry.is_regular_file())
		{
			auto ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
			if (ext == ".h" || ext == ".hpp")
			{
				headers.push_back(entry.path().string());
			}
		}
	}
}

int main(int argc, char* argv[])
{
	std::cout << "========================================\n";
	std::cout << " -- Welcome to use ElaineBuildTool --\n";
	std::cout << " ElaineBuildTool — Reflection Generator\n";
	std::cout << "========================================\n";

	CommandLineArgs args;
	if (!ParseCommandLine(argc, argv, args))
		return 1;

	// Setup parser
	ElaineBuildTool::HeaderParser parser;
	for (auto& inc : args.IncludeDirs)
	{
		parser.AddIncludePath(inc);
	}
	// Also add input dirs as include paths
	for (auto& inputDir : args.InputDirs)
	{
		parser.AddIncludePath(inputDir);
	}

	// Setup code generator
	ElaineBuildTool::CodeGenerator generator;
	generator.SetOutputDirectory(args.OutputDir);
	if (!args.EditorOutputDir.empty())
		generator.SetEditorOutputDirectory(args.EditorOutputDir);

	// Collect all headers
	std::vector<std::string> headers;
	for (auto& inputDir : args.InputDirs)
	{
		CollectHeaders(inputDir, headers);
	}

	std::cout << "[ElaineBuildTool] Found " << headers.size() << " header file(s) to scan.\n";

	// ---- Pass 1: parse all headers, collect reflected class names ----
	struct ParsedFileResult
	{
		std::string HeaderPath;
		std::vector<ElaineBuildTool::ParsedClass> Classes;
	};

	std::vector<ParsedFileResult> parsedFiles;
	std::unordered_set<std::string> reflectedClassNames;
	int errors = 0;

	for (auto& header : headers)
	{
		ParsedFileResult result;
		result.HeaderPath = header;

		if (!parser.ParseFile(header, result.Classes))
		{
			errors++;
			continue;
		}

		for (auto& cls : result.Classes)
			reflectedClassNames.insert(cls.ClassName);

		parsedFiles.push_back(std::move(result));
	}

	std::cout << "[ElaineBuildTool] Collected " << reflectedClassNames.size()
			  << " reflected class name(s) across all headers.\n";

	// Pass the full set to the generator so it knows which parents are reflected
	generator.SetReflectedClassNames(reflectedClassNames);

	// ---- Pass 2: generate code ----
	int totalClasses = 0;

	for (auto& pf : parsedFiles)
	{
		if (pf.Classes.empty())
			continue;

		totalClasses += static_cast<int>(pf.Classes.size());
		if (!generator.GenerateForClasses(pf.Classes, pf.HeaderPath))
		{
			errors++;
		}
		// Generate .generated.cpp (static registration)
		if (!generator.GenerateForClassesCpp(pf.Classes, pf.HeaderPath))
		{
			errors++;
		}
		// Also generate ImGui editor code
		if (!generator.GenerateEditorForClasses(pf.Classes, pf.HeaderPath))
		{
			errors++;
		}
		if (!generator.GenerateEditorForClassesCpp(pf.Classes, pf.HeaderPath))
		{
			errors++;
		}
	}

	std::cout << "\n========================================\n";
	std::cout << " Results: " << totalClasses << " reflected class(es), "
			  << errors << " error(s)\n";
	std::cout << "========================================\n";

	return errors > 0 ? 1 : 0;
}
