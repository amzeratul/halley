#pragma once
#include "../tasks/editor_task.h"
#include "import_assets_task.h"

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

		void checkAllAssets();
		void checkAssets(const std::vector<AssetToImport>& assets);
		void deleteMissing(const std::vector<Path>& paths);
	};
}
