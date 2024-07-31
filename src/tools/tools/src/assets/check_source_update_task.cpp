#include "halley/tools/assets/check_source_update_task.h"

#include "halley/concurrency/task_set.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/file/filesystem_cache.h"
#include "halley/tools/project/build_project_task.h"
#include "halley/tools/project/project.h"
#include "halley/utils/algorithm.h"

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

		updateHashes();

		if (lastSrcHash != lastSourceListsHash) {
			lastSourceListsHash = lastSrcHash;
			generateSourceListing();
		}

		if (lastSrcHash != lastReadFile) {
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

void CheckSourceUpdateTask::updateHashes()
{
	if (firstCheck || monitorSource.pollAny()) {
		lastSrcHash = toString(project.getSourceHash(), 16);
	}

	if (firstCheck || monitorCurrent.pollAny()) {
		lastReadFile = project.getBuiltSourceStr();
	}

	firstCheck = false;
}

void CheckSourceUpdateTask::generateSourceListing()
{
	const auto rootPath = project.getRootPath();
	const auto halleyPath = (project.getHalleyRootPath() / ".");
	const bool isEditor = rootPath == halleyPath;
	const auto projectPath = isEditor ? rootPath / "src" / "tools" / "editor" : rootPath;

	const auto srcListFile = projectPath / "source_list.txt";
	const auto cmakeLists = projectPath / "CMakeLists.txt";

	Vector<std::tuple<Path, Path, bool>> roots;
	roots.emplace_back(projectPath / "src", projectPath, true);
	if (isEditor) {
		roots.emplace_back(rootPath / "gen", projectPath, false);
	}
	//roots.emplace_back(halleyPath / "shared_gen", rootPath);

	Vector<String> result;

	for (const auto& [root, makeRelTo, acceptHeaders]: roots) {
		auto files = FileSystem::enumerateDirectory(root);
		//Logger::logInfo("Found " + toString(files.size()) + " files at " + root.getString());
		std::sort(files.begin(), files.end());

		for (auto& file: files) {
			const auto ext = file.getExtension();
			if (ext == ".cpp" || (acceptHeaders && (ext == ".h" || ext == ".hpp"))) {
				if (file.getFilenameStr() != "build_version.h") {
					auto path = (root / file).makeRelativeTo(makeRelTo);
					result.push_back(path.getString(false));
				}
			}
		}
	}

	if (!isEditor) {
		result.push_back("prec.h");
		result.push_back("prec.cpp");
	}

	auto prevContents = Path::readFileLines(srcListFile);
	std_ex::erase(prevContents, String());

	if (result != prevContents) {
		Logger::logInfo("Updating " + srcListFile.getString());
		setProgress(0.5f, "Updating source_list.txt");

		std::stringstream resultStr;
		constexpr const char* lineBreak = getPlatform() == GamePlatform::Windows ? "\r\n" : "\n";
		for (const auto& line: result) {
			resultStr << line.cppStr() << lineBreak;
		}
		auto newContents = String(resultStr.str());

		Path::writeFile(srcListFile, newContents);
		Path::touchFile(cmakeLists);
	} else {
		if (FileSystem::getLastWriteTime(srcListFile) > FileSystem::getLastWriteTime(cmakeLists)) {
			Logger::logInfo("Touching " + cmakeLists.getString());
			Path::touchFile(cmakeLists);
		}
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
