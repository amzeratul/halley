#include "import_assets_task.h"
#include "check_assets_task.h"
#include <thread>
#include "src/project/project.h"
#include "import_assets_database.h"

using namespace Halley;

ImportAssetsTask::ImportAssetsTask(Project& project, bool headless, Vector<AssetToImport>&& files)
	: EditorTask("Importing assets", true, true)
	, project(project)
	, headless(headless)
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

		try {
			importAsset(files[i]);
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

	if (!headless) {
		addContinuation(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(project, headless), 1.0f));
	}
}

void ImportAssetsTask::importAsset(AssetToImport& asset)
{
	auto src = asset.srcDir / asset.name;
	auto dst = project.getAssetsPath() / asset.name;
	auto root = asset.name.begin()->string();

	auto iter = importers.find(root);
	if (iter != importers.end()) {
		iter->second(src, dst);
	} else {
		// No importer found, just copy
		ensureParentDirectoryExists(dst);
		boost::filesystem::copy_file(src, dst, boost::filesystem::copy_option::overwrite_if_exists);
	}
}

void ImportAssetsTask::ensureParentDirectoryExists(Path path) const
{
	auto dstDir = boost::filesystem::is_directory(path) ? path : path.parent_path();
	if (!boost::filesystem::exists(dstDir)) {
		boost::filesystem::create_directories(dstDir);
	}
}

void ImportAssetsTask::setImportTable()
{
	
}
