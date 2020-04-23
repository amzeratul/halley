#pragma once
#include "halley/tools/tasks/editor_task.h"

namespace Halley {
	class Project;

	class BuildProjectTask : public EditorTask {
    public:
    	BuildProjectTask(Project& project);

    protected:
        void run() override;

	private:
		String command;
    };
}
