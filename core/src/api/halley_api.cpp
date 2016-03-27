#include "halley_api.h"

using namespace Halley;

HalleyAPI::HalleyAPI(CoreAPI* _core, std::unique_ptr<SystemAPI> _system, std::unique_ptr<VideoAPI> _video, std::unique_ptr<InputAPI> _input)
	: core(_core)
	, system(std::move(_system))
	, video(std::move(_video))
	, input(std::move(_input))
{
	if (system) {
		system->init();
	}
	if (video) {
		video->init();
	}
	if (input) {
		input->init();
	}
}

HalleyAPI::~HalleyAPI()
{
	if (input) {
		input->deInit();
	}
	if (video) {
		video->deInit();
	}
	if (system) {
		system->deInit();
	}
}

std::unique_ptr<HalleyAPI> HalleyAPI::create(CoreAPI* core, int flags)
{
	return std::unique_ptr<HalleyAPI>(new HalleyAPI(
		core,
		std::make_unique<SystemAPI>(),
		(flags & HalleyAPIFlags::Video) ? std::make_unique<VideoAPI>() : std::unique_ptr<VideoAPI>(),
		(flags & HalleyAPIFlags::Input) ? std::make_unique<InputAPI>() : std::unique_ptr<InputAPI>()
	));
}
