#pragma once
#include "../tasks/editor_task.h"
#include "import_assets_task.h"
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
		void checkAssets(const std::vector<AssetToImport>& assets);
		void deleteMissing(const std::vector<Path>& paths);
	};
}
