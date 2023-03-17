#include "check_update_task.h"

using namespace Halley;

CheckUpdateTask::CheckUpdateTask(Path projectPath)
	: Task("Check Update", true, false)
	, projectPath(std::move(projectPath))
	, monitorAssets(projectPath / "halley" / "include")
{
}

void CheckUpdateTask::run()
{
	while (!needsUpdate()) {
		if (isCancelled()) {
			return;
		}
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100ms);
	}

	addContinuation(std::make_unique<UpdateEditorTask>());
}

bool CheckUpdateTask::needsUpdate()
{
	if (firstCheck || monitorAssets.pollAny()) {
		firstCheck = false;
		return !versionMatches();
	}
	return false;
}

bool CheckUpdateTask::versionMatches()
{
	HalleyVersion projectVersion;
	projectVersion.parseHeader(Path::readFileLines(projectPath / "halley" / "include" / "halley_version.hpp"));
	return !projectVersion.isValid() || getHalleyVersion() == projectVersion;
}

UpdateEditorTask::UpdateEditorTask()
	: Task("Update Editor", false, true)
{
}

void UpdateEditorTask::run()
{
	logError("Editor needs updating.");
}
