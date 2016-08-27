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
		time_t fileTime;

		AssetToImport(Path name, Path srcDir, time_t time)
			: name(name)
			, srcDir(srcDir)
			, fileTime(time)
		{}
	};

	class ImportAssetsTask : public EditorTask
	{
	public:
		ImportAssetsTask(Project& project, Vector<AssetToImport>&& files);

	protected:
		void run() override;

	private:
		Project& project;
		Vector<AssetToImport> files;
	};
}
