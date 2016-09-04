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

		Path getGenPath() const;
		Path getGenSrcPath() const;

		ImportAssetsDatabase& getImportAssetsDatabase();
		ImportAssetsDatabase& getCodegenDatabase();
	
	private:
		Path rootPath;
		Path sharedAssetsSrcPath;

		std::unique_ptr<ImportAssetsDatabase> importAssetsDatabase;
		std::unique_ptr<ImportAssetsDatabase> codegenDatabase;
	};
}
