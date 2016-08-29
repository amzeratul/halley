#include <thread>
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/make_font/font_generator.h"
#include "halley/resources/resource_data.h"

using namespace Halley;

ImportAssetsTask::ImportAssetsTask(Project& project, bool headless, Vector<AssetToImport>&& files)
	: EditorTask("Importing assets", true, true)
	, project(project)
	, headless(headless)
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
		curFileLabel = files[i].name.filename().string();
		setProgress(curFileProgressStart, curFileLabel);

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

void ImportAssetsTask::ensureParentDirectoryExists(Path path)
{
	auto dstDir = boost::filesystem::is_directory(path) ? path : path.parent_path();
	if (!boost::filesystem::exists(dstDir)) {
		boost::filesystem::create_directories(dstDir);
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

		ensureParentDirectoryExists(dst);

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

		FontGenerator gen(false, [=] (float progress, String) {
			setProgress(lerp(curFileProgressStart, curFileProgressEnd, progress), curFileLabel);
		});
		gen.generateFont(src, dst.replace_extension("font"), imgSize, radius, supersample, Range<int>(0, 255));
	}
}

void ImportAssetsTask::setImportTable()
{
	importers["font"] = [this](Path src, Path dst) { loadFont(src, dst); };
}
