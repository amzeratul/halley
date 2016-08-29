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
		ImportAssetsTask(Project& project, Vector<ImportAssetsDatabaseEntry>&& files);

	protected:
		void run() override;

	private:
		Project& project;
		Vector<ImportAssetsDatabaseEntry> files;
		std::map<String, std::function<std::vector<Path>(const ImportAssetsDatabaseEntry&, Path)>> importers;
		
		float curFileProgressStart;
		float curFileProgressEnd;
		std::string curFileLabel;

		void importAsset(ImportAssetsDatabaseEntry& asset);
		static std::unique_ptr<Metadata> getMetaData(Path path);

		std::vector<Path> loadFont(const ImportAssetsDatabaseEntry& asset, Path dstDir);
		std::vector<Path> genericImporter(const ImportAssetsDatabaseEntry& asset, Path dstDir);

		void setImportTable();
	};
}
