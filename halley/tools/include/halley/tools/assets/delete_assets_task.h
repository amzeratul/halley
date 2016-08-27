#pragma once
#include "../tasks/editor_task.h"
#include <boost/filesystem/path.hpp>
#include "halley/data_structures/vector.h"

namespace Halley {
	class Project;

	using Path = boost::filesystem::path;

	class DeleteAssetsTask : public EditorTask
	{
	public:
		DeleteAssetsTask(Project& project, bool headless, Vector<Path> files);

	protected:
		void run() override;

	private:
		Project& project;
		bool headless;
		Vector<Path> files;
	};
}
