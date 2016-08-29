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
		CheckAssetsTask(Project& project);

	protected:
		void run() override;

	private:
		Project& project;
		DirectoryMonitor monitor;
		DirectoryMonitor monitorShared;

		void checkAllAssets();
		void checkAssets(const std::vector<ImportAssetsDatabaseEntry>& assets);
		void deleteMissing(const std::vector<ImportAssetsDatabaseEntry>& assets);
	};
}
