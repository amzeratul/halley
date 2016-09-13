#include <utility>
#include "api/halley_api.h"
#include <halley/plugin/plugin.h>

using namespace Halley;

HalleyAPI::HalleyAPI(CoreAPIInternal* _core, std::unique_ptr<SystemAPIInternal> _system, std::unique_ptr<VideoAPIInternal> _video, std::unique_ptr<InputAPIInternal> _input, std::unique_ptr<AudioAPIInternal> _audio)
	: coreInternal(_core)
	, systemInternal(std::move(_system))
	, videoInternal(std::move(_video))
	, inputInternal(std::move(_input))
	, audioInternal(std::move(_audio))
	, core(coreInternal)
	, system(&*systemInternal)
	, video(&*videoInternal)
	, input(&*inputInternal)
	, audio(&*audioInternal)
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
	if (audioInternal) {
		audioInternal->init();
	}
}

HalleyAPI::~HalleyAPI()
{
	if (audioInternal) {
		audioInternal->deInit();
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
	std::unique_ptr<SystemAPIInternal> system;
	{
		auto plugins = core->getPlugins(PluginType::SystemAPI);
		if (plugins.size() > 0) {
			system.reset(static_cast<SystemAPIInternal*>(plugins[0]->createAPI(nullptr)));
		} else {
			throw Exception("No suitable system plugins found.");
		}
	}

	std::unique_ptr<VideoAPIInternal> video;
	if (flags & HalleyAPIFlags::Video) {
		auto plugins = core->getPlugins(PluginType::GraphicsAPI);
		if (plugins.size() > 0) {
			video.reset(static_cast<VideoAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable video plugins found.");
		}
	}

	std::unique_ptr<InputAPIInternal> input;
	if (flags & HalleyAPIFlags::Input) {
		auto plugins = core->getPlugins(PluginType::InputAPI);
		if (plugins.size() > 0) {
			input.reset(static_cast<InputAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable input plugins found.");
		}
	}

	std::unique_ptr<AudioAPIInternal> audio;
	if (flags & HalleyAPIFlags::Audio) {
		auto plugins = core->getPlugins(PluginType::AudioAPI);
		if (plugins.size() > 0) {
			audio.reset(static_cast<AudioAPIInternal*>(plugins[0]->createAPI(system.get())));
		} else {
			throw Exception("No suitable audio plugins found.");
		}
	}

	return std::unique_ptr<HalleyAPI>(new HalleyAPI(core, std::move(system), std::move(video), std::move(input), std::move(audio)));
}
