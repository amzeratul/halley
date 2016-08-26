#include "check_assets_task.h"
#include "import_assets_task.h"
#include "../../project/project.h"
#include <unordered_set>

using namespace Halley;

CheckAssetsTask::CheckAssetsTask(Project& project)
	: EditorTask("Check assets", false, false)
	, project(project)
{}

void CheckAssetsTask::run()
{
	Vector<AssetToImport> filesToImport;
	std::unordered_set<std::string> included;

	// Enumerate all potential assets
	for (auto srcPath : { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }) {
		if (boost::filesystem::exists(srcPath)) {
			using RDI = boost::filesystem::recursive_directory_iterator;
			RDI end;
			for (RDI i(srcPath); i != end; ++i) {
				Path rawPath = i->path();
				if (boost::filesystem::is_regular_file(rawPath)) {
					Path filePath = rawPath.lexically_relative(srcPath);

					std::string filename = filePath.filename().string();
					if (included.find(filename) == included.end()) {
						included.insert(filename);
						filesToImport.push_back(AssetToImport(filePath, srcPath));
					}
				}
			}
		}
	}

	if (!filesToImport.empty()) {
		addContinuation(EditorTaskAnchor(std::make_unique<ImportAssetsTask>(project, std::move(filesToImport), project.getAssetsPath())));
	} else {
		// Schedule the next one to run after one second
		addContinuation(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(project), 1.0f));
	}
}
