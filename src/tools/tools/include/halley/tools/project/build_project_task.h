#pragma once
#include "halley/support/logger.h"
#include "halley/tools/tasks/editor_task.h"

namespace Halley {
	class Project;

	class BuildProjectTask : public EditorTask, ILoggerSink {
    public:
    	BuildProjectTask(Project& project);

    protected:
        void run() override;
        void log(LoggerLevel level, const String& msg) override;
		
	private:
		enum class BuildSystem {
			Unknown,
			MSBuild
		};
		
		Project& project;
		String command;
		BuildSystem buildSystem;

		void tryToIdentifyBuildSystem(const String& msg);
		void parseMSBuildMessage(const String& msg);
    };
}
