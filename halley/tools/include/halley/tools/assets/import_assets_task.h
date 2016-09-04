#pragma once
#include "halley/tools/tasks/editor_task.h"
#include "halley/file/filesystem.h"
#include <vector>
#include "halley/resources/metadata.h"
#include "import_assets_database.h"

namespace Halley
{
	class Project;
	
	class ImportAssetsTask : public EditorTask
	{
	public:
		ImportAssetsTask(ImportAssetsDatabase& db, const AssetImporter& importer, Path assetsPath, Vector<ImportAssetsDatabaseEntry>&& files);

	protected:
		void run() override;

	private:
		ImportAssetsDatabase& db;
		const AssetImporter& importer;
		Path assetsPath;

		Vector<ImportAssetsDatabaseEntry> files;
		
		float curFileProgressStart = 0;
		float curFileProgressEnd = 0;
		std::string curFileLabel;

		void importAsset(ImportAssetsDatabaseEntry& asset);

		std::vector<Path> loadFont(const ImportAssetsDatabaseEntry& asset, Path dstDir);
		std::vector<Path> genericImporter(const ImportAssetsDatabaseEntry& asset, Path dstDir);
	};
}
