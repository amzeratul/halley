#pragma once
#include "../editor_task.h"
#include <boost/filesystem.hpp>

namespace Halley
{
	class Project;
	using Path = boost::filesystem::path;

	class AssetToImport
	{
	public:
		Path name;
		Path srcDir;

		AssetToImport(Path name, Path srcDir)
			: name(name)
			, srcDir(srcDir)
		{}
	};

	class ImportAssetsTask : public EditorTask
	{
	public:
		ImportAssetsTask(Project& project, Vector<AssetToImport>&& files, Path destinationFolder);

	protected:
		void run() override;

	private:
		Project& project;
		Vector<AssetToImport> files;
		Path destinationFolder;
	};
}
