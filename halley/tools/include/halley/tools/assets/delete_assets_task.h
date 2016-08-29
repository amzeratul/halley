#pragma once
#include "../tasks/editor_task.h"
#include "halley/data_structures/vector.h"
#include "halley/file/filesystem.h"

namespace Halley {
	class Project;

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
