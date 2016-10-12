#include "halley/tools/assets/import_tool.h"
#include "halley/tools/codegen/codegen.h"
#include <iostream>
#include "halley/tools/project/project.h"
#include "halley/tools/tasks/editor_task_set.h"
#include "halley/tools/assets/check_assets_task.h"
#include <thread>
#include <chrono>
#include "halley/tools/file/filesystem.h"
#include "halley/os/os.h"

using namespace Halley;
using namespace std::chrono_literals;

int ImportTool::run(Vector<std::string> args)
{
	if (args.size() == 2) {
		Executors executors;
		Executors::set(executors);
		OS::setInstance(OS::createOS());
		ThreadPool tp(executors.getCPU(), std::max(static_cast<unsigned int>(4), std::thread::hardware_concurrency()));

		Path projectPath = FileSystem::getAbsolute(Path(args[0]));
		Path sharedAssetsPath = FileSystem::getAbsolute(Path(args[1]) / "assets_src");

		auto proj = std::make_unique<Project>(projectPath, sharedAssetsPath);
		std::cout << "Importing project at " << projectPath << ", with shared assets at " << sharedAssetsPath << "" << std::endl;

		auto tasks = std::make_unique<EditorTaskSet>();
		tasks->addTask(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(*proj, true)));
		auto last = std::chrono::steady_clock::now();

		while (tasks->getTasks().size() > 0) {
			std::this_thread::sleep_for(50ms);

			auto now = std::chrono::steady_clock::now();
			float elapsed = std::chrono::duration<float>(now - last).count();
			last = now;

			tasks->update(elapsed);
		}

		std::cout << "Import done." << std::endl;
		return 0;
	}
	else {
		std::cout << "Usage: halley-cmd import projDir halleyDir" << std::endl;
		return 1;
	}
}
