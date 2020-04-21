#include <utility>
#include "api/halley_api.h"
#include <halley/plugin/plugin.h>
#include "halley/audio/audio_facade.h"

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

	std::unique_ptr<SystemAPIInternal> system;
	{
		auto plugins = core->getPlugins(PluginType::SystemAPI);
		if (!plugins.empty()) {
			std::cout << "System plugin: " << plugins[0]->getName() << "\n";
			system.reset(dynamic_cast<SystemAPIInternal*>(plugins[0]->createAPI(nullptr)));
		} else {
			throw Exception("No suitable system plugins found.", HalleyExceptions::Core);
		}
	}

	if (flags & HalleyAPIFlags::Video) {
		auto plugins = core->getPlugins(PluginType::GraphicsAPI);
		if (!plugins.empty()) {
			std::cout << "Video plugin: " << plugins[0]->getName() << "\n";
			api->videoInternal.reset(dynamic_cast<VideoAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable video plugins found.", HalleyExceptions::Core);
		}
	}

	if (flags & HalleyAPIFlags::Input) {
		auto plugins = core->getPlugins(PluginType::InputAPI);
		if (!plugins.empty()) {
			std::cout << "Input plugin: " << plugins[0]->getName() << "\n";
			api->inputInternal.reset(dynamic_cast<InputAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable input plugins found.", HalleyExceptions::Core);
		}
	}

	if (flags & HalleyAPIFlags::Audio) {
		auto plugins = core->getPlugins(PluginType::AudioOutputAPI);
		if (!plugins.empty()) {
			std::cout << "Audio output plugin: " << plugins[0]->getName() << "\n";
			api->audioOutputInternal.reset(dynamic_cast<AudioOutputAPIInternal*>(plugins[0]->createAPI(system.get())));
			api->audioInternal = std::make_unique<AudioFacade>(*api->audioOutputInternal, *system);
		} else {
			throw Exception("No suitable audio plugins found.", HalleyExceptions::Core);
		}
	}

	if (flags & HalleyAPIFlags::Network) {
		auto plugins = core->getPlugins(PluginType::NetworkAPI);
		if (!plugins.empty()) {
			std::cout << "Network plugin: " << plugins[0]->getName() << "\n";
			api->networkInternal.reset(dynamic_cast<NetworkAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable network plugins found.", HalleyExceptions::Core);
		}
	}

	if (flags & HalleyAPIFlags::Platform) {
		auto plugins = core->getPlugins(PluginType::PlatformAPI);
		if (!plugins.empty()) {
			std::cout << "Platform plugin: " << plugins[0]->getName() << "\n";
			api->platformInternal.reset(dynamic_cast<PlatformAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable network plugins found.", HalleyExceptions::Core);
		}
	}

	if (flags & HalleyAPIFlags::Movie) {
		auto plugins = core->getPlugins(PluginType::MovieAPI);
		if (!plugins.empty()) {
			std::cout << "Movie plugin: " << plugins[0]->getName() << "\n";
			api->movieInternal.reset(dynamic_cast<MovieAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable movie plugins found.", HalleyExceptions::Core);
		}
	}

	api->systemInternal = std::move(system);
	api->assign();
	return api;
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
