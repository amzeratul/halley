#include <utility>
#include "api/halley_api.h"
#include <halley/plugin/plugin.h>
#include "halley/audio/audio_facade.h"

using namespace Halley;

HalleyAPI::~HalleyAPI()
{
	deInit();
}

void HalleyAPI::assign()
{
	Expects(coreInternal);
	Expects(systemInternal);
	core = coreInternal;

	if (systemInternal) {
		system = systemInternal.get();
	}
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
}

void HalleyAPI::deInit()
{
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
	auto api = std::unique_ptr<HalleyAPI>(new HalleyAPI());
	api->coreInternal = core;

	std::unique_ptr<SystemAPIInternal> system;
	{
		auto plugins = core->getPlugins(PluginType::SystemAPI);
		if (plugins.size() > 0) {
			std::cout << "System plugin: " << plugins[0]->getName() << "\n";
			system.reset(static_cast<SystemAPIInternal*>(plugins[0]->createAPI(nullptr)));
		} else {
			throw Exception("No suitable system plugins found.");
		}
	}

	if (flags & HalleyAPIFlags::Video) {
		auto plugins = core->getPlugins(PluginType::GraphicsAPI);
		if (plugins.size() > 0) {
			std::cout << "Video plugin: " << plugins[0]->getName() << "\n";
			api->videoInternal.reset(static_cast<VideoAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable video plugins found.");
		}
	}

	if (flags & HalleyAPIFlags::Input) {
		auto plugins = core->getPlugins(PluginType::InputAPI);
		if (plugins.size() > 0) {
			std::cout << "Input plugin: " << plugins[0]->getName() << "\n";
			api->inputInternal.reset(static_cast<InputAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable input plugins found.");
		}
	}

	if (flags & HalleyAPIFlags::Audio) {
		auto plugins = core->getPlugins(PluginType::AudioOutputAPI);
		if (plugins.size() > 0) {
			std::cout << "Audio output plugin: " << plugins[0]->getName() << "\n";
			api->audioOutputInternal.reset(static_cast<AudioOutputAPIInternal*>(plugins[0]->createAPI(system.get())));
			api->audioInternal = std::make_unique<AudioFacade>(*api->audioOutputInternal, *system);
		} else {
			throw Exception("No suitable audio plugins found.");
		}
	}

	if (flags & HalleyAPIFlags::Network) {
		auto plugins = core->getPlugins(PluginType::NetworkAPI);
		if (plugins.size() > 0) {
			std::cout << "Network plugin: " << plugins[0]->getName() << "\n";
			api->networkInternal.reset(static_cast<NetworkAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable network plugins found.");
		}
	}

	if (flags & HalleyAPIFlags::Platform) {
		auto plugins = core->getPlugins(PluginType::PlatformAPI);
		if (plugins.size() > 0) {
			std::cout << "Platform plugin: " << plugins[0]->getName() << "\n";
			api->platformInternal.reset(static_cast<PlatformAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable network plugins found.");
		}
	}

	api->systemInternal = std::move(system);
	api->assign();
	return std::move(api);
}
