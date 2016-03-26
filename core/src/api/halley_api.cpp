#include "halley_api.h"

using namespace Halley;

HalleyAPI::HalleyAPI(std::unique_ptr<CoreAPI> _core, std::unique_ptr<VideoAPI> _video, std::unique_ptr<InputAPI> _input)
	: core(std::move(_core))
	, video(std::move(_video))
	, input(std::move(_input))
{
	if (core) {
		core->init();
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
	if (core) {
		core->deInit();
	}
}

std::unique_ptr<HalleyAPI> HalleyAPI::create(int flags)
{
	return std::unique_ptr<HalleyAPI>(new HalleyAPI(
		(flags & HalleyAPIFlags::Core) ? std::make_unique<CoreAPI>() : std::unique_ptr<CoreAPI>(),
		(flags & HalleyAPIFlags::Video) ? std::make_unique<VideoAPI>() : std::unique_ptr<VideoAPI>(),
		(flags & HalleyAPIFlags::Input) ? std::make_unique<InputAPI>() : std::unique_ptr<InputAPI>()
	));
}
