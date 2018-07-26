#include <halley/support/exception.h>
#include "halley/tools/cli_tool.h"
#include "halley/tools/codegen/codegen_tool.h"
#include "halley/tools/distance_field/distance_field_tool.h"
#include "halley/tools/make_font/make_font_tool.h"
#include "halley/tools/assets/import_tool.h"
#include "halley/tools/packer/asset_packer_tool.h"
#include "halley/support/logger.h"
#include "halley/core/game/halley_statics.h"
#include "halley/tools/vs_project/vs_project_tool.h"

using namespace Halley;

CommandLineTools::CommandLineTools()
{
	factories["import"] = []() { return std::make_unique<ImportTool>(); };
	factories["codegen"] = []() { return std::make_unique<CodegenTool>(); };
	factories["distField"] = []() { return std::make_unique<DistanceFieldTool>(); };
	factories["makeFont"] = []() { return std::make_unique<MakeFontTool>(); };
	factories["pack"] = []() { return std::make_unique<AssetPackerTool>(); };
	factories["vs_project"] = []() { return std::make_unique<VSProjectTool>(); };
}

Vector<std::string> CommandLineTools::getToolNames()
{
	auto result = Vector<std::string>();
	for (auto& kv : factories) {
		result.push_back(kv.first);
	}
	return result;
}

std::unique_ptr<CommandLineTool> CommandLineTools::getTool(std::string name)
{
	auto result = factories.find(name);
	if (result != factories.end()) {
		return result->second();
	} else {
		throw Exception("Unknown tool name");
	}
}

CommandLineTool::~CommandLineTool()
{
}

int CommandLineTool::runRaw(int argc, char** argv)
{
	platform = "pc";

	Vector<std::string> args;
	for (int i = 0; i < argc; i++) {
		String arg = argv[i];
		if (arg.startsWith("--")) {
			if (arg.startsWith("--platform=")) {
				platform = arg.mid(11);
			}
		} else {
			args.push_back(arg.cppStr());
		}
	}

	statics = std::make_unique<HalleyStatics>();
	statics->resume(nullptr);
	StdOutSink logSink(true);
	Logger::addSink(logSink);

	return run(args);
}
