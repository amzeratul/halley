#include "avf_movie_api.h"
#include "avf_movie_player.h"
using namespace Halley;

AVFMovieAPI::AVFMovieAPI(SystemAPI& system)
	: system(system)
{
}

void AVFMovieAPI::init()
{
}

void AVFMovieAPI::deInit()
{
}

std::shared_ptr<MoviePlayer> AVFMovieAPI::makePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data)
{
	return std::make_shared<AVFMoviePlayer>(video, audio, data);
}
