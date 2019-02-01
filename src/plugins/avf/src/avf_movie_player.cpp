#include "avf_movie_player.h"

using namespace Halley;

AVFMoviePlayer::AVFMoviePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data)
	: MoviePlayer(video, audio)
	, data(std::move(data))
{
}

AVFMoviePlayer::~AVFMoviePlayer() noexcept
{
	reset();
}

void AVFMoviePlayer::requestVideoFrame()
{
}

void AVFMoviePlayer::requestAudioFrame()
{
}

void AVFMoviePlayer::onReset()
{
}
