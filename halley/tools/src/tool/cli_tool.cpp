#include <halley/support/exception.h>
#include "halley/tools/cli_tool.h"
#include "halley/tools/codegen/codegen_tool.h"
#include "halley/tools/distance_field/distance_field_tool.h"
#include "halley/tools/make_font/make_font_tool.h"

using namespace Halley;

CommandLineTools::CommandLineTools()
{
	factories["codegen"] = []() { return std::make_unique<CodegenTool>(); };
	factories["distField"] = []() { return std::make_unique<DistanceFieldTool>(); };
	factories["makeFont"] = []() { return std::make_unique<MakeFontTool>(); };
}

Vector<std::string> CommandLineTools::getToolNames()
{
	auto result = Vector<std::string>(factories.size());
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

int CommandLineTool::runRaw(int argc, char** argv)
{
	Vector<std::string> args(argc);
	for (int i = 0; i < argc; i++) {
		args[i] = argv[i];
	}
	return run(args);
}
