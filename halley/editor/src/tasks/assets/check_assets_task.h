#pragma once
#include "../editor_task.h"

namespace Halley
{
	class Project;

	class CheckAssetsTask : public EditorTask
	{
	public:
		CheckAssetsTask(Project& project);

	protected:
		void run() override;

	private:
		Project& project;
	};
}
