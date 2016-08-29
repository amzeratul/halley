#pragma once

#include "halley/file/filesystem.h"

namespace Halley
{
	class ImportAssetsDatabase;

	class Project
	{
	public:
		Project(Path rootPath, Path sharedAssetsSrcPath);
		~Project();
		
		Path getAssetsPath() const;
		Path getAssetsSrcPath() const;
		Path getSharedAssetsSrcPath() const;

		ImportAssetsDatabase& getImportAssetsDatabase();
	
	private:
		Path rootPath;
		Path sharedAssetsSrcPath;

		std::unique_ptr<ImportAssetsDatabase> importAssetsDatabase;
	};
}
