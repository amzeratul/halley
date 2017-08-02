#pragma once
#include "../tasks/editor_task.h"
#include "import_assets_database.h"
#include "halley/file/directory_monitor.h"

namespace Halley
{
	class Project;

	class CheckAssetsTask : public EditorTask
	{
	public:
		CheckAssetsTask(Project& project, bool headless);

	protected:
		void run() override;

	private:
		Project& project;
		DirectoryMonitor monitorAssets;
		DirectoryMonitor monitorAssetsSrc;
		DirectoryMonitor monitorSharedAssetsSrc;
		DirectoryMonitor monitorGen;
		DirectoryMonitor monitorGenSrc;
		bool headless;

		static std::vector<ImportAssetsDatabaseEntry> filterNeedsImporting(ImportAssetsDatabase& db, const std::map<String, ImportAssetsDatabaseEntry>& assets);
		void checkAllAssets(ImportAssetsDatabase& db, std::vector<Path> srcPaths, Path dstPath, String taskName);
		Maybe<Path> findDirectoryMeta(const std::vector<Path>& metas, const Path& path) const;
	};
}
