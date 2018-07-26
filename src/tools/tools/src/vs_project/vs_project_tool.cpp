#include "halley/tools/vs_project/vs_project_tool.h"
#include "halley/support/logger.h"
#include "halley/support/exception.h"
#include "halley/tools/vs_project/vs_project_manipulator.h"
#include "halley/tools/file/filesystem.h"
#include "halley/file/path.h"
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
				throw Exception("Invalid switch: " + arg);
			}
		} else {
			if (mode == Mode::CollectingInput) {
				input.push_back(arg);
			} else if (mode == Mode::CollectingOutput) {
				if (output.isEmpty()) {
					output = arg;
				} else {
					throw Exception("Output already specified");
				}
			} else {
				throw Exception("Unknown parameter");
			}
		}
	}

	std::set<String> compile;
	std::set<String> include;

	const auto outputPath = Path(output);
	const auto outputDir = outputPath.parentPath();

	for (auto& in: input) {
		const auto inputPath = Path(in);

		VSProjectManipulator inProj;
		inProj.load(FileSystem::readFile(inputPath));

		for (auto& i: inProj.getCompileFiles()) {
			compile.insert(Path(i).changeRelativeRoot(inputPath, outputDir).getString().replaceAll("/", "\\"));
		}
		for (auto& i: inProj.getIncludeFiles()) {
			include.insert(Path(i).changeRelativeRoot(inputPath, outputDir).getString().replaceAll("/", "\\"));
		}
	}
	
	VSProjectManipulator outProj;
	outProj.load(FileSystem::readFile(outputPath));
	outProj.setCompileFiles(compile);
	outProj.setIncludeFiles(include);
	FileSystem::writeFile(outputPath.string(), outProj.write());

	return 0;
}
