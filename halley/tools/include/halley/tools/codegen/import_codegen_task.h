#pragma once
#include "halley/tools/tasks/editor_task.h"
#include "halley/file/filesystem.h"
#include "halley/tools/assets/import_assets_database.h"

namespace Halley
{
	class Project;

	class ImportCodegenTask : public EditorTask
	{
	public:
		ImportCodegenTask(ImportAssetsDatabase& db, Path assetsPath, Vector<ImportAssetsDatabaseEntry>&& files);

	protected:
		void run() override;

	private:
		ImportAssetsDatabase& db;
		Vector<ImportAssetsDatabaseEntry> files;
		Path srcPath;
		Path dstPath;
	};
}
