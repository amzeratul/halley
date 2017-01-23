#include "dummy_plugins.h"
#include "dummy_audio.h"
#include "dummy_system.h"
#include "dummy_video.h"
#include "dummy_input.h"

using namespace Halley;

PluginType DummyAudioPlugin::getType()
{
	return PluginType::AudioOutputAPI;
}

String DummyAudioPlugin::getName()
{
	return "dummyAudio";
}

HalleyAPIInternal* DummyAudioPlugin::createAPI(SystemAPI*)
{
	return new DummyAudioAPI();
}

int DummyAudioPlugin::getPriority() const
{
	return -1;
}

PluginType DummySystemPlugin::getType()
{
	return PluginType::SystemAPI;
}

String DummySystemPlugin::getName()
{
	return "dummySystem";
}

HalleyAPIInternal* DummySystemPlugin::createAPI(SystemAPI*)
{
	return new DummySystemAPI();
}

int DummySystemPlugin::getPriority() const
{
	return -1;
}

PluginType DummyVideoPlugin::getType()
{
	return PluginType::GraphicsAPI;
}

String DummyVideoPlugin::getName()
{
	return "dummyVideo";
}

HalleyAPIInternal* DummyVideoPlugin::createAPI(SystemAPI* system)
{
	return new DummyVideoAPI(*system);
}

int DummyVideoPlugin::getPriority() const
{
	return -1;
}

PluginType DummyInputPlugin::getType()
{
	return PluginType::InputAPI;
}

String DummyInputPlugin::getName()
{
	return "dummyInput";
}

HalleyAPIInternal* DummyInputPlugin::createAPI(SystemAPI*)
{
	return new DummyInputAPI();
}

int DummyInputPlugin::getPriority() const
{
	return -1;
}
