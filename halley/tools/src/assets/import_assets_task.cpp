#include <thread>
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/make_font/font_generator.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

ImportAssetsTask::ImportAssetsTask(Project& project, Vector<ImportAssetsDatabaseEntry>&& files)
	: EditorTask("Importing assets", true, true)
	, project(project)
	, files(std::move(files))
{}

void ImportAssetsTask::run()
{
	setImportTable();

	using namespace std::chrono_literals;
	auto& db = project.getImportAssetsDatabase();
	auto lastSave = std::chrono::steady_clock::now();
	auto destinationFolder = project.getAssetsPath();

	for (size_t i = 0; i < files.size(); ++i) {
		if (isCancelled()) {
			break;
		}

		curFileProgressStart = float(i) / float(files.size());
		curFileProgressEnd = float(i + 1) / float(files.size());
		curFileLabel = files[i].inputFile.filename().string();
		setProgress(curFileProgressStart, curFileLabel);

		try {
			importAsset(files[i]);
			if (isCancelled()) {
				break;
			}
			db.markAsImported(files[i]);

			// Check if db needs saving
			auto now = std::chrono::steady_clock::now();
			if (now - lastSave > 1s) {
				db.save();
				lastSave = now;
			}
		} catch (std::exception& e) {
			std::cout << "Error importing asset " << files[i].inputFile << ": " << e.what() << std::endl;
		}
	}
	db.save();

	if (!isCancelled()) {
		setProgress(1.0f, "");
	}
}

void ImportAssetsTask::importAsset(ImportAssetsDatabaseEntry& asset)
{
	auto src = asset.srcDir / asset.inputFile;
	auto dst = project.getAssetsPath() / asset.inputFile;
	auto root = asset.inputFile.begin()->string();

	auto iter = importers.find(root);
	if (iter != importers.end()) {
		iter->second(src, dst);
	} else {
		// No importer found, just copy
		FileSystem::copyFile(src, dst);
	}
}

std::unique_ptr<Metadata> ImportAssetsTask::getMetaData(Path path)
{
	try {
		return std::make_unique<Metadata>(*ResourceDataStatic::loadFromFileSystem(path.string() + ".meta"));
	} catch (...) {
		return std::unique_ptr<Metadata>();
	}
}

void ImportAssetsTask::loadFont(Path src, Path dst)
{
	if (src.extension() != ".meta") {
		std::cout << "Importing font " << src << std::endl;

		FileSystem::createParentDir(dst);

		Vector2i imgSize(512, 512);
		float radius = 8;
		int supersample = 4;
		auto meta = getMetaData(src);
		if (meta) {
			radius = meta->getFloat("radius", 8);
			supersample = meta->getInt("supersample", 4);
			imgSize.x = meta->getInt("width", 512);
			imgSize.y = meta->getInt("height", 512);
		}

		FontGenerator gen(false, [=] (float progress, String) -> bool {
			setProgress(lerp(curFileProgressStart, curFileProgressEnd, progress), curFileLabel);
			return !isCancelled();
		});
		gen.generateFont(src, dst.replace_extension("font"), imgSize, radius, supersample, Range<int>(0, 255));
	}
}

void ImportAssetsTask::setImportTable()
{
	importers["font"] = [this](Path src, Path dst) { loadFont(src, dst); };
}
