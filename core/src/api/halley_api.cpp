#include "halley_api.h"
#include "halley_api_internal.h"
#include "../sdl/system_sdl.h"
#include "../input/input.h"

using namespace Halley;

HalleyAPI::HalleyAPI(CoreAPIInternal* _core, std::unique_ptr<SystemAPIInternal> _system, std::unique_ptr<VideoAPIInternal> _video, std::unique_ptr<InputAPIInternal> _input)
	: coreInternal(_core)
	, systemInternal(std::move(_system))
	, videoInternal(std::move(_video))
	, inputInternal(std::move(_input))
	, core(coreInternal)
	, system(&*systemInternal)
	, video(&*videoInternal)
	, input(&*inputInternal)
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
}

HalleyAPI::~HalleyAPI()
{
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
	std::unique_ptr<VideoAPIInternal> video;
	if (flags & HalleyAPIFlags::Video) {
		auto plugins = Plugin::getPlugins(PluginType::GraphicsAPI);
		if (plugins.size() > 0) {
			video.reset(static_cast<VideoAPIInternal*>(plugins[0]->createAPI()));
		} else {
			throw Exception("No suitable video plugins found.");
		}
	}
	
	return std::unique_ptr<HalleyAPI>(new HalleyAPI(core, std::unique_ptr<SystemSDL>(new SystemSDL()), std::move(video), std::unique_ptr<InputAPIInternal>(new Input())));
}
