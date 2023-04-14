#include "core_api_wrapper.h"

#include "halley/stage/stage.h"

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

bool CoreAPIWrapper::isDevMode()
{
	return parent.isDevMode();
}

void CoreAPIWrapper::addProfilerCallback(IProfileCallback* callback)
{
	parent.addProfilerCallback(callback);
}

void CoreAPIWrapper::removeProfilerCallback(IProfileCallback* callback)
{
	parent.removeProfilerCallback(callback);
}

void CoreAPIWrapper::addStartFrameCallback(IStartFrameCallback* callback)
{
	parent.addStartFrameCallback(callback);
}

void CoreAPIWrapper::removeStartFrameCallback(IStartFrameCallback* callback)
{
	parent.removeStartFrameCallback(callback);
}

Future<std::unique_ptr<RenderSnapshot>> CoreAPIWrapper::requestRenderSnapshot()
{
	return parent.requestRenderSnapshot();
}

DevConClient* CoreAPIWrapper::getDevConClient() const
{
	return nullptr;
}
