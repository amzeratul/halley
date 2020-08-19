#include "core_api_wrapper.h"

#include "halley/core/stage/stage.h"

using namespace Halley;

CoreAPIWrapper::CoreAPIWrapper(CoreAPI& parent)
	: parent(parent)
{
}

void CoreAPIWrapper::registerPlugin(std::unique_ptr<Plugin> plugin)
{
	// Do nothing
}

Vector<Plugin*> CoreAPIWrapper::getPlugins(PluginType type)
{
	throw Exception("Unable to retrieve plugins from CoreAPIWrapper", HalleyExceptions::Tools);
}

void CoreAPIWrapper::quit(int exitCode)
{
	// Do nothing
}

void CoreAPIWrapper::setStage(StageID stage)
{
	// Do nothing
}

void CoreAPIWrapper::setStage(std::unique_ptr<Stage> stage)
{
	// Do nothing
}

void CoreAPIWrapper::initStage(Stage& stage)
{
	// Do nothing
}

Stage& CoreAPIWrapper::getCurrentStage()
{
	throw Exception("Unable to retrieve current stage from CoreAPIWrapper", HalleyExceptions::Tools);
}

HalleyStatics& CoreAPIWrapper::getStatics()
{
	return parent.getStatics();
}

const Environment& CoreAPIWrapper::getEnvironment()
{
	return parent.getEnvironment();
}

int64_t CoreAPIWrapper::getTime(CoreAPITimer timer, TimeLine tl, StopwatchRollingAveraging::Mode mode) const
{
	return parent.getTime(timer, tl, mode);
}

void CoreAPIWrapper::setTimerPaused(CoreAPITimer timer, TimeLine tl, bool paused)
{
	parent.setTimerPaused(timer, tl, paused);
}

bool CoreAPIWrapper::isDevMode()
{
	return parent.isDevMode();
}
