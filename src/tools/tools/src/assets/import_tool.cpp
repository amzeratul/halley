#include "halley/tools/assets/import_tool.h"
#include "halley/tools/codegen/codegen.h"
#include "halley/tools/project/project.h"
#include "halley/tools/tasks/editor_task_set.h"
#include "halley/tools/assets/check_assets_task.h"
#include <thread>
#include <chrono>
#include "halley/tools/file/filesystem.h"
#include "halley/os/os.h"
#include "halley/core/game/halley_statics.h"
#include "halley/support/logger.h"
#include "halley/tools/project/project_loader.h"

using namespace Halley;
using namespace std::chrono_literals;

int ImportTool::run(Vector<std::string> args)
{
	if (args.size() >= 2) {
		const Path projectPath = FileSystem::getAbsolute(Path(args[0]));
		const Path halleyRootPath = FileSystem::getAbsolute(Path(args[1]));
		
		ProjectLoader loader(*statics, halleyRootPath);
		loader.setPlatform(platform);
		auto proj = loader.loadProject(projectPath);

		if (args.size() >= 3) {
			const Path manifestPath = FileSystem::getAbsolute(Path(args[2]));
			proj->setAssetPackManifest(manifestPath);
		}
		Logger::logInfo("Importing project at \"" + projectPath + "\", with Halley root at \"" + halleyRootPath + "\" and manifest at \"" + proj->getAssetPackManifestPath() + "\"");

		auto tasks = std::make_unique<EditorTaskSet>();
		tasks->setListener(*this);
		tasks->addTask(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(*proj, true)));
		auto last = std::chrono::steady_clock::now();

		while (!tasks->getTasks().empty()) {
			std::this_thread::sleep_for(50ms);

			auto now = std::chrono::steady_clock::now();
			float elapsed = std::chrono::duration<float>(now - last).count();
			last = now;

			tasks->update(elapsed);
		}

		if (hasError) {
			Logger::logError("Import failed.");
			return 1;
		} else {
			Logger::logInfo("Import done.");
			return 0;
		}
	} else {
		Logger::logError("Usage: halley-cmd import projDir halleyDir");
		return 1;
	}
}

void ImportTool::onTaskAdded(const std::shared_ptr<EditorTaskAnchor>& task)
{
	Logger::logInfo("Task added: " + task->getName());
}

void ImportTool::onTaskTerminated(const std::shared_ptr<EditorTaskAnchor>& task)
{
	Logger::logInfo("Task ended: " + task->getName());
}

void ImportTool::onTaskError(const std::shared_ptr<EditorTaskAnchor>& task)
{
	Logger::logError("Task ended in error: " + task->getName() + ": " + task->getError());
	hasError = true;
}
