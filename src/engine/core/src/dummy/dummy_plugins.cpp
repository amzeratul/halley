#include "dummy_plugins.h"
#include "dummy_audio.h"
#include "dummy_system.h"
#include "dummy_video.h"
#include "dummy_input.h"
#include "dummy_network.h"
#include "dummy_platform.h"

using namespace Halley;

PluginType DummyAudioPlugin::getType()
{
	return PluginType::AudioOutputAPI;
}

String DummyAudioPlugin::getName()
{
	return "Audio/Dummy";
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
	return "System/Dummy";
}

HalleyAPIInternal* DummySystemPlugin::createAPI(SystemAPI*)
{
	return new DummySystemAPI();
}

int DummySystemPlugin::getPriority() const
{
	return -1;
}

PluginType DummyNetworkPlugin::getType()
{
	return PluginType::NetworkAPI;
}

String DummyNetworkPlugin::getName()
{
	return "Network/Dummy";
}

HalleyAPIInternal* DummyNetworkPlugin::createAPI(SystemAPI*)
{
	return new DummyNetworkAPI();
}

int DummyNetworkPlugin::getPriority() const
{
	return -1;
}

PluginType DummyPlatformPlugin::getType()
{
	return PluginType::PlatformAPI;
}

String DummyPlatformPlugin::getName()
{
	return "Platform/Dummy";
}

HalleyAPIInternal* DummyPlatformPlugin::createAPI(SystemAPI*)
{
	return new DummyPlatformAPI();
}

int DummyPlatformPlugin::getPriority() const
{
	return -1;
}

PluginType DummyVideoPlugin::getType()
{
	return PluginType::GraphicsAPI;
}

String DummyVideoPlugin::getName()
{
	return "Video/Dummy";
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
	return "Input/Dummy";
}

HalleyAPIInternal* DummyInputPlugin::createAPI(SystemAPI*)
{
	return new DummyInputAPI();
}

int DummyInputPlugin::getPriority() const
{
	return -1;
}
