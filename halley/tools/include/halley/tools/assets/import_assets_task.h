#pragma once
#include "halley/tools/tasks/editor_task.h"
#include <boost/filesystem.hpp>
#include <map>
#include <functional>
#include "halley/resources/metadata.h"

namespace Halley
{
	class Project;
	using Path = boost::filesystem::path;

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
		ImportAssetsTask(Project& project, bool headless, Vector<AssetToImport>&& files);

	protected:
		void run() override;

	private:
		Project& project;
		bool headless;
		Vector<AssetToImport> files;
		std::map<String, std::function<void(Path, Path)>> importers;

		void importAsset(AssetToImport& asset);
		static void ensureParentDirectoryExists(Path path);
		static std::unique_ptr<Metadata> getMetaData(Path path);

		void loadFont(Path src, Path dst) const;

		void setImportTable();
	};
}
