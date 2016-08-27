#pragma once
#include "src/tasks/editor_task.h"
#include <boost/filesystem/path.hpp>

namespace Halley {
	class Project;

	using Path = boost::filesystem::path;

	class DeleteAssetsTask : public EditorTask
	{
	public:
		DeleteAssetsTask(Project& project, Vector<Path> files);

	protected:
		void run() override;

	private:
		Project& project;
		Vector<Path> files;
	};
}
