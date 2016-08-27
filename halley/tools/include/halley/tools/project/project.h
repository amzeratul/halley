#pragma once

#include <boost/filesystem.hpp>

namespace Halley
{
	using Path = boost::filesystem::path;

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
