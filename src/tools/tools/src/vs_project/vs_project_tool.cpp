#include "halley/tools/vs_project/vs_project_tool.h"
#include "halley/support/logger.h"
#include "halley/support/exception.h"
#include "halley/tools/vs_project/vs_project_manipulator.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/path.h"
#include "halley/core/game/environment.h"
#include "halley/os/os.h"
using namespace Halley;

int VSProjectTool::run(Vector<std::string> args)
{
	if (args.empty()) {
		Logger::logInfo("Please specify vs_project verb. Available: copy_files");
		return 1;
	} else {
		if (args[0] == "copy_files") {
			args.erase(args.begin());
			return copyFiles(args);
		} else {
			Logger::logInfo("Invalid vs_project verb. Available: copy_files");
			return 1;
		}
	}
}

int VSProjectTool::copyFiles(const std::vector<std::string>& args)
{
	std::vector<String> input;
	String output;

	enum class Mode {
		Idle,
		CollectingInput,
		CollectingOutput
	};
	Mode mode = Mode::Idle;

	for (auto& _arg: args) {
		const String arg = _arg;
		if (arg.startsWith("-")) {
			if (arg == "-input") {
				mode = Mode::CollectingInput;
			} else if (arg == "-output") {
				mode = Mode::CollectingOutput;
			} else {
				throw Exception("Invalid switch: " + arg, HalleyExceptions::Tools);
			}
		} else {
			if (mode == Mode::CollectingInput) {
				input.push_back(arg);
			} else if (mode == Mode::CollectingOutput) {
				if (output.isEmpty()) {
					output = arg;
				} else {
					throw Exception("Output already specified", HalleyExceptions::Tools);
				}
			} else {
				throw Exception("Unknown parameter", HalleyExceptions::Tools);
			}
		}
	}

	const auto rootPath = Path(OS::get().getCurrentWorkingDir());

	std::set<String> compile;
	std::set<String> include;

	const auto outputPath = Path(output).isAbsolute() ? Path(output) : rootPath / Path(output);
	const auto outputDir = outputPath.parentPath();
	Logger::logInfo("Updating project file: " + outputPath.string());

	for (auto& in: input) {
		const auto inputPath = Path(in).isAbsolute() ? Path(in) : rootPath / Path(in);

		VSProjectManipulator inProj;
		inProj.load(inputPath);

		for (auto& i: inProj.getCompileFiles()) {
			compile.insert(Path(i).changeRelativeRoot(inputPath, outputDir).getString().replaceAll("/", "\\"));
		}
		for (auto& i: inProj.getIncludeFiles()) {
			include.insert(Path(i).changeRelativeRoot(inputPath, outputDir).getString().replaceAll("/", "\\"));
		}
	}
	
	VSProjectManipulator outProj;
	outProj.load(outputPath);
	outProj.setCompileFiles(compile);
	outProj.setIncludeFiles(include);
	FileSystem::writeFile(outputPath.string(), outProj.write());

	return 0;
}
