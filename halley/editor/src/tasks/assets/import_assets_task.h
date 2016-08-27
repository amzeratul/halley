#pragma once
#include "../editor_task.h"
#include <boost/filesystem.hpp>
#include <map>
#include <functional>

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
		void ensureParentDirectoryExists(Path path) const;

		void setImportTable();
	};
}
