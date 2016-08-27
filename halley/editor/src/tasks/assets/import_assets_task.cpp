#include "import_assets_task.h"
#include "check_assets_task.h"
#include <thread>
#include "src/project/project.h"
#include "import_assets_database.h"

using namespace Halley;

ImportAssetsTask::ImportAssetsTask(Project& project, Vector<AssetToImport>&& files)
	: EditorTask("Importing assets", true, true)
	, project(project)
	, files(std::move(files))
{}

void ImportAssetsTask::run()
{
	using namespace std::chrono_literals;
	auto& db = project.getImportAssetsDatabase();
	auto lastSave = std::chrono::steady_clock::now();
	auto destinationFolder = project.getAssetsPath();

	for (size_t i = 0; i < files.size(); ++i) {
		if (isCancelled()) {
			break;
		}
		setProgress(float(i) / float(files.size()), files[i].name.filename().string());

		// Copy file for now
		try {
			auto dst = destinationFolder / files[i].name;
			auto dstDir = dst.parent_path();
			if (!boost::filesystem::exists(dstDir)) {
				boost::filesystem::create_directories(dstDir);
			}
			boost::filesystem::copy_file(files[i].srcDir / files[i].name, dst, boost::filesystem::copy_option::overwrite_if_exists);

			// Mark file as imported
			db.markAsImported(files[i].name, files[i].fileTime);

			// Check if db needs saving
			auto now = std::chrono::steady_clock::now();
			if (now - lastSave > 1s) {
				db.save();
				lastSave = now;
			}
		} catch (std::exception& e) {
			std::cout << "Error importing asset " << files[i].name << ": " << e.what() << std::endl;
		}
	}
	db.save();

	if (!isCancelled()) {
		setProgress(1.0f, "");
	}

	addContinuation(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(project), 1.0f));
}
