#pragma once
#include "halley/tools/tasks/editor_task.h"
#include "halley/file/filesystem.h"
#include <map>
#include <functional>
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
		std::map<String, std::function<void(Path, Path)>> importers;
		
		float curFileProgressStart;
		float curFileProgressEnd;
		std::string curFileLabel;

		void importAsset(ImportAssetsDatabaseEntry& asset);
		static std::unique_ptr<Metadata> getMetaData(Path path);

		void loadFont(Path src, Path dst);

		void setImportTable();
	};
}
