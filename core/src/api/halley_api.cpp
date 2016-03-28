#include "halley_api.h"
#include "halley_api_internal.h"
#include "../opengl/video_opengl.h"
#include "../sdl/system_sdl.h"

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
	return std::unique_ptr<HalleyAPI>(new HalleyAPI(
		core,
		std::unique_ptr<SystemSDL>(new SystemSDL()),
		(flags & HalleyAPIFlags::Video) ? std::unique_ptr<VideoAPIInternal>(new VideoOpenGL()) : std::unique_ptr<VideoAPIInternal>(),
		(flags & HalleyAPIFlags::Input) ? std::unique_ptr<InputAPIInternal>(/*todo*/) : std::unique_ptr<InputAPIInternal>()
	));
}
