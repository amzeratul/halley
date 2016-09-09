#include "halley/tools/assets/import_tool.h"
#include "halley/tools/codegen/codegen.h"
#include <iostream>
#include "halley/tools/project/project.h"
#include "halley/tools/tasks/editor_task_set.h"
#include "halley/tools/assets/check_assets_task.h"
#include <thread>
#include <chrono>

using namespace Halley;
using namespace std::chrono_literals;

int ImportTool::run(Vector<std::string> args)
{
	if (args.size() == 2) {
		std::cout << "Importing project..." << std::endl;
		Executors executors;
		Executors::set(executors);
		ThreadPool tp(executors.getCPU(), 4);

		Path sharedAssetsPath = Path(args[1]) / "assets_src";
		auto proj = std::make_unique<Project>(args[0], sharedAssetsPath);

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

		return 0;
	}
	else {
		std::cout << "Usage: halley-cmd import projDir halleyDir" << std::endl;
		return 1;
	}
}
