#include <utility>
#include "api/halley_api.h"
#include <halley/plugin/plugin.h>
#include "halley/audio/audio_facade.h"
#include "halley/support/logger.h"
#include "entry/entry_point.h"

using namespace Halley;

void HalleyAPI::assign()
{
	Expects(coreInternal != nullptr);
	Expects(systemInternal != nullptr);

	core = coreInternal;
	system = systemInternal.get();

	if (videoInternal) {
		video = videoInternal.get();
	}
	if (inputInternal) {
		input = inputInternal.get();
	}
	if (audioInternal) {
		audio = audioInternal.get();
	}
	if (platformInternal) {
		platform = platformInternal.get();
	}
	if (networkInternal) {
		network = networkInternal.get();
	}
	if (movieInternal) {
		movie = movieInternal.get();
	}
}

void HalleyAPI::init()
{
	if (systemInternal) {
		systemInternal->init();
	}
	if (videoInternal) {
		videoInternal->init();
	}
	if (inputInternal) {
		inputInternal->init();
	}
	if (audioOutputInternal) {
		audioOutputInternal->init();
	}
	if (audioInternal) {
		audioInternal->init();
	}
	if (platformInternal) {
		platformInternal->init();
	}
	if (networkInternal) {
		networkInternal->init();
	}
	if (movieInternal) {
		movieInternal->init();
	}
}

void HalleyAPI::deInit()
{
	if (movieInternal) {
		movieInternal->deInit();
	}
	if (networkInternal) {
		networkInternal->deInit();
	}
	if (platformInternal) {
		platformInternal->deInit();
	}
	if (audioInternal) {
		audioInternal->deInit();
	}
	if (audioOutputInternal) {
		audioOutputInternal->deInit();
	}
	if (inputInternal) {
		inputInternal->deInit();
	}
	if (videoInternal) {
		videoInternal->deInit();
	}
	if (systemInternal) {
		systemInternal->deInit();
	}
}

std::unique_ptr<HalleyAPI> HalleyAPI::create(CoreAPIInternal* core, int flags)
{
	auto api = std::make_unique<HalleyAPI>();
	api->coreInternal = core;

	HalleyAPIFlags::Flags flagList[] = { HalleyAPIFlags::System, HalleyAPIFlags::Video, HalleyAPIFlags::Input, HalleyAPIFlags::Audio, HalleyAPIFlags::Network, HalleyAPIFlags::Platform, HalleyAPIFlags::Movie };
	PluginType pluginTypes[] = { PluginType::SystemAPI, PluginType::GraphicsAPI, PluginType::InputAPI, PluginType::AudioOutputAPI, PluginType::NetworkAPI, PluginType::PlatformAPI, PluginType::MovieAPI };
	String names[] = { "System", "Graphics", "Input", "AudioOutput", "Network", "Platform", "Movie" };

	constexpr size_t n = std::end(flagList) - std::begin(flagList);
	for (size_t i = 0; i < n; ++i) {
		if (flags & flagList[i] || flagList[i] == HalleyAPIFlags::System) {
			auto plugins = core->getPlugins(pluginTypes[i]);
			if (!plugins.empty()) {
				Logger::logInfo(names[i] + " plugin: " + plugins[0]->getName());
				api->setAPI(pluginTypes[i], plugins[0]->createAPI(api->systemInternal.get()));
			} else {
				throw Exception("No suitable " + names[i] + " plugins found.", HalleyExceptions::Core);
			}
		}
	}

	api->assign();
	return api;
}

void HalleyAPI::setAPI(PluginType pluginType, HalleyAPIInternal* api)
{
	switch (pluginType) {
	case PluginType::SystemAPI:
		systemInternal.reset(dynamic_cast<SystemAPIInternal*>(api));
		return;
	case PluginType::InputAPI:
		inputInternal.reset(dynamic_cast<InputAPIInternal*>(api));
		return;
	case PluginType::GraphicsAPI:
		videoInternal.reset(dynamic_cast<VideoAPIInternal*>(api));
		return;
	case PluginType::AudioOutputAPI:
		audioOutputInternal.reset(dynamic_cast<AudioOutputAPIInternal*>(api));
		audioInternal = std::make_unique<AudioFacade>(*audioOutputInternal, *systemInternal);
		return;
	case PluginType::PlatformAPI:
		platformInternal.reset(dynamic_cast<PlatformAPIInternal*>(api));
		return;
	case PluginType::NetworkAPI:
		networkInternal.reset(dynamic_cast<NetworkAPIInternal*>(api));
		return;
	case PluginType::MovieAPI:
		movieInternal.reset(dynamic_cast<MovieAPIInternal*>(api));
		return;
	}
}

HalleyAPI& HalleyAPI::operator=(const HalleyAPI& other) = default;

std::unique_ptr<HalleyAPI> HalleyAPI::clone() const
{
	auto api = std::make_unique<HalleyAPI>();
	*api = *this;
	return api;
}

void HalleyAPI::replaceCoreAPI(CoreAPIInternal* coreAPI)
{
	coreInternal = coreAPI;
	core = coreAPI;
}

uint32_t Halley::getHalleyDLLAPIVersion()
{
	return 138;
}
