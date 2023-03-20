#include "halley/tools/project/write_version_tool.h"

#include "halley/support/logger.h"
#include "halley/version/version.h"
#include "halley/tools/file/filesystem.h"

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
