#pragma once
#include "../tasks/editor_task.h"
#include "halley/data_structures/vector.h"
#include "halley/file/path.h"
#include "import_assets_database.h"

namespace Halley {
	class Project;

	class DeleteAssetsTask : public EditorTask
	{
	public:
		DeleteAssetsTask(ImportAssetsDatabase& db, Path assetsPath, Vector<ImportAssetsDatabaseEntry> assets);

	protected:
		void run() override;

	private:
		ImportAssetsDatabase& db;
		Path assetsPath;
		Vector<ImportAssetsDatabaseEntry> assets;
	};
}
