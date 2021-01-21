#pragma once
#include "halley/support/logger.h"
#include "halley/concurrency/task.h"
#include <regex>

namespace Halley {
	class Project;

	class BuildProjectTask : public Task, ILoggerSink {
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

		LoggerLevel lastLevel = LoggerLevel::Info;

		void tryToIdentifyBuildSystem(const String& msg);
		LoggerLevel parseMSBuildMessage(LoggerLevel level, const String& msg);
		LoggerLevel parseNinjaMessage(LoggerLevel level, const String& msg);
    };
}
