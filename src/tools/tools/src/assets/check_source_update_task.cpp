#include "halley/tools/assets/check_source_update_task.h"

#include "halley/concurrency/task_set.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/file/filesystem_cache.h"
#include "halley/tools/project/build_project_task.h"
#include "halley/tools/project/project.h"

using namespace Halley;

CheckSourceUpdateTask::CheckSourceUpdateTask(Project& project, bool autoBuild, bool oneShot)
	: Task("Check Source Update", true, false)
	, project(project)
	, autoBuild(autoBuild)
	, oneShot(oneShot)
	, monitorSource(project.getRootPath() / "src")
	, monitorCurrent(project.getRootPath() / "bin")
{}

void CheckSourceUpdateTask::run()
{
	while (!isCancelled()) {
		using namespace std::chrono_literals;

		if (needsUpdate()) {
			generateSourceListing();

			if (!project.isBuildPending()) {
				project.onBuildStarted();
				if (autoBuild) {
					addPendingTask(std::make_unique<BuildProjectTask>(project));
				} else {
					addPendingTask(std::make_unique<UpdateSourceTask>(project, !oneShot));
				}
			}
		}

		while (hasPendingTasks()) {
			std::this_thread::sleep_for(5ms);
		}

		if (oneShot) {
			return;
		}
		std::this_thread::sleep_for(100ms);
	}
}

bool CheckSourceUpdateTask::needsUpdate()
{
	if (firstCheck || monitorSource.pollAny()) {
		lastSrcHash = toString(project.getSourceHash(), 16);
	}

	if (firstCheck || monitorCurrent.pollAny()) {
		lastReadFile = project.getBuiltSourceStr();
	}

	firstCheck = false;

	if (lastSrcHash != lastReadFile) {
		//Logger::logInfo("Src hashes to " + lastSrcHash + ", file reads " + lastReadFile);
	}

	return lastSrcHash != lastReadFile;
}

void CheckSourceUpdateTask::generateSourceListing()
{
	const auto rootPath = project.getRootPath();
	const auto halleyPath = (project.getHalleyRootPath() / ".");
	const bool isEditor = rootPath == halleyPath;
	const auto projectPath = isEditor ? rootPath / "src" / "tools" / "editor" : rootPath;

	const auto srcListFile = projectPath / "source_list.txt";
	const auto cmakeLists = projectPath / "CMakeLists.txt";

	std::stringstream result;

	Vector<std::pair<Path, Path>> roots;
	roots.emplace_back(projectPath / "src", projectPath);
	roots.emplace_back(rootPath / "gen", projectPath);
	//roots.emplace_back(halleyPath / "shared_gen", rootPath);

	for (const auto& [root, makeRelTo]: roots) {
		auto files = FileSystem::enumerateDirectory(root);
		//Logger::logInfo("Found " + toString(files.size()) + " files at " + root.getString());
		std::sort(files.begin(), files.end());

		for (auto& file: files) {
			const auto ext = file.getExtension();
			if (ext == ".h" || ext == ".cpp" || ext == ".hpp") {
				auto path = (root / file).makeRelativeTo(makeRelTo);
				result << path.getString(false).cppStr() << "\n";
			}
		}
	}

	auto newContents = String(result.str());
	auto curContents = Path::readFileString(srcListFile);

	if (newContents != curContents) {
		Logger::logInfo("Updating " + srcListFile.getString());
		setProgress(0.5f, "Updating source_list.txt");

		Path::writeFile(srcListFile, newContents);
		Path::touchFile(cmakeLists);
	}
}



UpdateSourceTask::UpdateSourceTask(Project& project, bool reportError)
	: Task("Update Project", false, true)
	, project(project)
	, reportError(reportError)
{
}

void UpdateSourceTask::run()
{
	if (reportError) {
		logError("Project needs building");
	} else {
		setProgress(1.0f);
	}
}

std::optional<String> UpdateSourceTask::getAction()
{
	if (reportError) {
		return "Build";
	}
	return std::nullopt;
}

void UpdateSourceTask::doAction(TaskSet& taskSet)
{
	Concurrent::execute(Executors::getMainUpdateThread(), [this, &taskSet, &project = project]()
	{
		taskSet.addTask(std::make_unique<BuildProjectTask>(project));
		setError(false);
		setVisible(false);
	});
}
