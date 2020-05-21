#pragma once
#include "halley/support/logger.h"
#include "halley/tools/tasks/editor_task.h"
#include <regex>

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
			MSBuild,
			Ninja,
			Make
		};
		
		Project& project;
		String command;
		BuildSystem buildSystem = BuildSystem::Unknown;

		std::regex matchProgress;

		void tryToIdentifyBuildSystem(const String& msg);
		void parseMSBuildMessage(const String& msg);
		void parseNinjaMessage(const String& msg);
    };
}
