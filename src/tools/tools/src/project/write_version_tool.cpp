#include "halley/tools/project/write_version_tool.h"

#include "halley/support/logger.h"
#include "halley/text/string_converter.h"
#include "halley/version/version.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/project/project.h"
#include "halley/utils/hash.h"

using namespace Halley;

int WriteVersionTool::run(Vector<std::string> args)
{
	const Path projectPath = FileSystem::getAbsolute(Path(args.at(0)));
	std::cout << "Writing version to " << (projectPath / "halley" / "bin" / "build_version.txt").string() << std::endl;
	std::cout << "Version is " << getHalleyVersion().toString() << std::endl;

	bool ok = Path::writeFile(projectPath / "halley" / "bin" / "build_version.txt", getHalleyVersion().toString());
	if (!ok) {
		Logger::logError("Failed to write build_version.txt!");
	}

	return 0;
}

int WriteCodeVersionTool::run(Vector<std::string> args)
{
	const Path projectPath = FileSystem::getAbsolute(Path(args.at(0)));

	const auto version = toString(Project::getSourceHash(projectPath), 16);

	std::cout << "Writing code version to " << (projectPath / "bin" / "code_version.txt").string() << std::endl;
	std::cout << "Version is " << version << std::endl;

	bool ok = Path::writeFile(projectPath / "bin" / "code_version.txt", version);
	if (!ok) {
		Logger::logError("Failed to write code_version.txt!");
	}

	return 0;
}
