#pragma once
#include "halley/tools/tasks/editor_task.h"
#include "halley/file/filesystem.h"
#include <map>
#include <functional>
#include <vector>
#include "halley/resources/metadata.h"
#include "import_assets_database.h"

namespace Halley
{
	class Project;
	
	class ImportAssetsTask : public EditorTask
	{
	public:
		ImportAssetsTask(ImportAssetsDatabase& db, Path assetsPath, Vector<ImportAssetsDatabaseEntry>&& files);

	protected:
		void run() override;

	private:
		ImportAssetsDatabase& db;
		Path assetsPath;

		Vector<ImportAssetsDatabaseEntry> files;
		std::map<String, std::function<std::vector<Path>(const ImportAssetsDatabaseEntry&, Path)>> importers;
		
		float curFileProgressStart = 0;
		float curFileProgressEnd = 0;
		std::string curFileLabel;

		void importAsset(ImportAssetsDatabaseEntry& asset);
		static std::unique_ptr<Metadata> getMetaData(Path path);

		std::vector<Path> loadFont(const ImportAssetsDatabaseEntry& asset, Path dstDir);
		std::vector<Path> genericImporter(const ImportAssetsDatabaseEntry& asset, Path dstDir);

		void setImportTable();
	};
}
