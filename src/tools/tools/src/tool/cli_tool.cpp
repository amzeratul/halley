#include <halley/support/exception.h>
#include "halley/tools/cli_tool.h"
#include "halley/tools/codegen/codegen_tool.h"
#include "halley/tools/distance_field/distance_field_tool.h"
#include "halley/tools/make_font/make_font_tool.h"
#include "halley/tools/assets/import_tool.h"
#include "halley/tools/packer/asset_packer_tool.h"
#include "halley/support/logger.h"
#include "halley/game/halley_statics.h"
#include "halley/support/debug.h"
#include "halley/tools/vs_project/vs_project_tool.h"
#include "halley/tools/packer/asset_pack_inspector.h"
#include "halley/tools/project/write_version_tool.h"
#include "halley/tools/runner/runner_tool.h"

using namespace Halley;

CommandLineTools::CommandLineTools()
{
	factories["import"] = []() { return std::make_unique<ImportTool>(); };
	factories["codegen"] = []() { return std::make_unique<CodegenTool>(); };
	factories["distField"] = []() { return std::make_unique<DistanceFieldTool>(); };
	factories["makeFont"] = []() { return std::make_unique<MakeFontTool>(); };
	factories["pack"] = []() { return std::make_unique<AssetPackerTool>(); };
	factories["pack-inspector"] = []() { return std::make_unique<AssetPackInspectorTool>(); };
	factories["vs_project"] = []() { return std::make_unique<VSProjectTool>(); };
	factories["run"] = []() { return std::make_unique<RunnerTool>(); };
	factories["write_version"] = []() { return std::make_unique<WriteVersionTool>(); };
	factories["write_code_version"] = []() { return std::make_unique<WriteCodeVersionTool>(); };
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
		throw Exception("Unknown tool name", HalleyExceptions::Tools);
	}
}

CommandLineTool::~CommandLineTool()
{
}

int CommandLineTool::runRaw(int argc, char** argv)
{
	Vector<std::string> args;
	for (int i = 2; i < argc; i++) {
		String arg = argv[i];
		if (!arg.startsWith("--")) {
			args.push_back(arg.cppStr());
		}
	}

	statics = std::make_unique<HalleyStatics>();
	statics->resume(nullptr, std::thread::hardware_concurrency());
	StdOutSink logSink(true, true);
	Logger::addSink(logSink);
	env.parseProgramPath(argv[0]);

	std::cout << "Running with args: ";
	for (auto& arg: args) {
		std::cout << "\"" << arg << "\" ";
	}
	std::cout << std::endl;

	Debug::setErrorHandling("stack.dmp", [=](const std::string& error) { onTerminatedInError(error); });
	
	const auto result = run(args);
	std::cout << "halley-cmd done." << std::endl;
	return result;
}

int CommandLineTool::run(Vector<std::string> args)
{
	throw Exception("Tool's run() method not implemented", HalleyExceptions::Tools);
}

void CommandLineTool::onTerminatedInError(const std::string& error)
{
	Logger::logError("Terminating in error: " + error);
	std::cout.flush();
}
