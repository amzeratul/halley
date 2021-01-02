#include "halley/tools/project/build_project_task.h"
#include "halley/core/game/game_platform.h"
#include "halley/os/os.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"
#include <chrono>

#include "halley/support/debug.h"

using namespace Halley;

BuildProjectTask::BuildProjectTask(Project& project)
	: EditorTask("Building project", true, true)
	, project(project)
{
	const String scriptName = [] ()
	{
		if constexpr (getPlatform() == GamePlatform::Windows) {
			return "build_project_win.bat";
			//return "build_project_win_ninja.bat";
		} else if constexpr (getPlatform() == GamePlatform::MacOS) {
			return "build_project_mac.sh";
		} else if constexpr (getPlatform() == GamePlatform::Linux) {
			return "build_project_linux.sh";
		} else {
			throw Exception("No project build script available for this platform.", HalleyExceptions::Tools);
		}
	}();
	const auto buildScript = project.getHalleyRootPath() / "scripts" / scriptName;
	const String buildConfig = Debug::isDebug() ? "Debug" : "RelWithDebInfo";
	command = "\"" + buildScript + "\" \"" + project.getRootPath() + "\" " + project.getProperties().getBinName() + " " + buildConfig;
}

void BuildProjectTask::run()
{
	using namespace std::literals::chrono_literals;
	auto future = OS::get().runCommandAsync(command, this);

	while (!future.isReady()) {
		if (isCancelled()) {
			future.cancel();
			return;
		}
		std::this_thread::sleep_for(10ms);
	}

	const int returnValue = future.get();
	if (returnValue == 0) {
		// Success
		project.onBuildDone();
	} else {
		// Fail
		addError("Script returned error code " + toString(returnValue));
	}
}

void BuildProjectTask::log(LoggerLevel level, const String& msg)
{
	if (level == LoggerLevel::Error) {
		addError(msg);
	}

	if (buildSystem == BuildSystem::Unknown) {
		tryToIdentifyBuildSystem(msg);
	}

	switch (buildSystem) {
	case BuildSystem::MSBuild:
		parseMSBuildMessage(msg);
		break;

	case BuildSystem::Ninja:
		parseNinjaMessage(msg);
		break;
	}

	Logger::logDev(msg);
}

void BuildProjectTask::tryToIdentifyBuildSystem(const String& msg)
{
	if (msg.contains("Microsoft (R) Build Engine")) {
		buildSystem = BuildSystem::MSBuild;
	}
	
	if (msg.startsWith("Configuring Ninja") || msg.startsWith("Building with Ninja")) {
		buildSystem = BuildSystem::Ninja;
		matchProgress = std::regex("\\[(\\d+)\\/(\\d+)\\]");
	}
}

void BuildProjectTask::parseMSBuildMessage(const String& rawMsg)
{
	String msg = rawMsg;
	msg.trimBoth();

	if (msg.endsWith(".c") || msg.endsWith(".cpp")) {
		// Current file
		setProgress(0, msg);
	}

	if (msg.contains("->")) {
		// Linking
		auto split = msg.split("->");
		if (split.size() >= 2) {
			split.back().trimBoth();
			setProgress(0, Path(split.back()).getFilename().toString());
		}
	}
}

void BuildProjectTask::parseNinjaMessage(const String& rawMsg)
{
	String msg = rawMsg;
	msg.trimBoth();

	std::smatch baseMatch;
	if (std::regex_search(rawMsg.cppStr(), baseMatch, matchProgress)) {
		if (baseMatch.size() == 3) {
			const int n = String(baseMatch[1]).toInteger();
			const int total = String(baseMatch[2]).toInteger();

			const auto split = rawMsg.split(' ');
			auto fileName = Path(split.back()).getFilename().toString();
			if (fileName.endsWith(".obj")) {
				fileName = fileName.left(fileName.size() - 4);
			}
			setProgress(static_cast<float>(n) / static_cast<float>(total), fileName);
		}
	}
}
