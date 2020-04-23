#include "halley/tools/project/build_project_task.h"
#include "halley/core/game/game_platform.h"
#include "halley/os/os.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"

using namespace Halley;

BuildProjectTask::BuildProjectTask(Project& project)
	: EditorTask("Building game", true, true)
{
	const String scriptName = [] ()
	{
		if constexpr (getPlatform() == GamePlatform::Windows) {
			return "build_project_win.bat";
		} else if constexpr (getPlatform() == GamePlatform::MacOS) {
			return "build_project_mac.sh";
		} else if constexpr (getPlatform() == GamePlatform::Linux) {
			return "build_project_linux.sh";
		} else {
			throw Exception("No project build script available for this platform.", HalleyExceptions::Tools);
		}
	}();
	const auto buildScript = project.getHalleyRootPath() / "scripts" / scriptName;
	command = "\"" + buildScript + "\" \"" + project.getRootPath() + "\" " + project.getProperties().getBinName();
}

void BuildProjectTask::run()
{
	OS::get().runCommand(command);
}
