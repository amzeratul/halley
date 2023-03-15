#include "halley/tools/project/write_version_tool.h"

#include "halley/version/version.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

int WriteVersionTool::run(Vector<std::string> args)
{
	const Path projectPath = FileSystem::getAbsolute(Path(args[0]));

	Path::writeFile(projectPath / "halley" / "bin" / "build_version.txt", getHalleyVersion().toString());

	return 0;
}
