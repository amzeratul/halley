#pragma once
#include "halley/tools/tasks/editor_task.h"
#include "halley/file/filesystem.h"
#include <map>
#include <functional>
#include "halley/resources/metadata.h"

namespace Halley
{
	class Project;

	class AssetToImport
	{
	public:
		Path name;
		Path srcDir;
		time_t fileTime;

		AssetToImport(Path name, Path srcDir, time_t time)
			: name(name)
			, srcDir(srcDir)
			, fileTime(time)
		{}
	};

	class ImportAssetsTask : public EditorTask
	{
	public:
		ImportAssetsTask(Project& project, Vector<AssetToImport>&& files);

	protected:
		void run() override;

	private:
		Project& project;
		Vector<AssetToImport> files;
		std::map<String, std::function<void(Path, Path)>> importers;
		
		float curFileProgressStart;
		float curFileProgressEnd;
		std::string curFileLabel;

		void importAsset(AssetToImport& asset);
		static std::unique_ptr<Metadata> getMetaData(Path path);

		void loadFont(Path src, Path dst);

		void setImportTable();
	};
}
