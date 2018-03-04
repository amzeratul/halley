#include "halley/core/graphics/movie/movie_player.h"
#include "resources/resources.h"
#include "halley/concurrency/concurrent.h"
#include "halley/core/graphics/texture_descriptor.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/api/video_api.h"
#include "halley/core/graphics/material/material_definition.h"
#include "../../../../audio/src/audio_handle_impl.h"
#include "halley/audio/audio_clip.h"

using namespace Halley;

MoviePlayer::MoviePlayer(VideoAPI& video, AudioAPI& audio)
	: video(video)
	, audio(audio)
{}

void MoviePlayer::play()
{
	if (state == MoviePlayerState::Paused) {
		state = MoviePlayerState::StartingToPlay;

		for (auto& s: streams) {
			s.playing = true;
		}
	}
}

void MoviePlayer::pause()
{
	if (state == MoviePlayerState::Playing) {
		state = MoviePlayerState::Paused;

		for (auto& s: streams) {
			s.playing = false;
		}

		audio.stopMusic();
	}
}

void MoviePlayer::reset()
{
	state = MoviePlayerState::Paused;
	time = 0;

	currentTexture.reset();
	pendingFrames.clear();
	streamingClip.reset();
	audio.stopMusic();

	onReset();
}

void MoviePlayer::update(Time t)
{
	if (state == MoviePlayerState::Playing) {
		time += t;
	}

	if (state == MoviePlayerState::Playing || state == MoviePlayerState::StartingToPlay) {
		MoviePlayerStream* videoStream = nullptr;
		MoviePlayerStream* audioStream = nullptr;
		for (auto& s: streams) {
			if (s.type == MoviePlayerStreamType::Video) {
				videoStream = &s;
			} else if (s.type == MoviePlayerStreamType::Audio) {
				audioStream = &s;
			}
		}

		bool readyToPlay = true;

		if (videoStream) {
			if (pendingFrames.size() < 5 && !videoStream->eof) {
				requestVideoFrame();
				readyToPlay = false;
			}

			if (!pendingFrames.empty()) {
				auto& next = pendingFrames.front();
				if (!currentTexture || time >= next.time) {
					if (currentTexture) {
						//recycleTexture.push_back(currentTexture);
					}
					currentTexture = next.texture;
					pendingFrames.pop_front();
				}
			}

			if (pendingFrames.empty() && videoStream->eof) {
				state = MoviePlayerState::Finished;
			}
		}

		if (audioStream) {
			if (!streamingClip) {
				streamingClip = std::make_shared<StreamingAudioClip>(2);
			}
			if (streamingClip->getSamplesLeft() < 10000) {
				requestAudioFrame();
				readyToPlay = false;
			}
		}

		if (readyToPlay && state == MoviePlayerState::StartingToPlay) {
			if (streamingClip) {
				audio.playMusic(streamingClip)->setGain(0.5f);
			}
			state = MoviePlayerState::Playing;
		}
	}
}

Sprite MoviePlayer::getSprite(Resources& resources)
{
	if (state == MoviePlayerState::Playing || state == MoviePlayerState::Paused) {
		auto matDef = resources.get<MaterialDefinition>("Halley/NV12Video");
		return Sprite().setImage(currentTexture, matDef).setTexRect(Rect4f(0, 0, 1, 1)).setSize(Vector2f(videoSize));
	} else {
		return Sprite().setMaterial(resources, "Halley/SolidColour").setColour(Colour4f(0, 1, 0)).setSize(Vector2f(videoSize));
	}
}

MoviePlayerState MoviePlayer::getState() const
{
	return state;
}

Vector2i MoviePlayer::getSize() const
{
	return videoSize;
}

void MoviePlayer::setVideoSize(Vector2i size)
{
	videoSize = size;
}

VideoAPI& MoviePlayer::getVideoAPI() const
{
	return video;
}

AudioAPI& MoviePlayer::getAudioAPI() const
{
	return audio;
}

void MoviePlayer::onVideoFrameAvailable(Time time, TextureDescriptor&& descriptor)
{
	std::shared_ptr<Texture> tex;
	if (recycleTexture.empty()) {
		tex = video.createTexture(descriptor.size);
	} else {
		tex = recycleTexture.front();
		recycleTexture.pop_front();
	}
	tex->startLoading();
	pendingFrames.push_back({tex, time});

	auto desc = std::make_shared<TextureDescriptor>(std::move(descriptor));

	Concurrent::execute(Executors::getVideoAux(), [tex, desc(std::move(desc))] ()
	{
		tex->load(TextureDescriptor(std::move(*desc)));
	});
}

void MoviePlayer::onAudioFrameAvailable(Time time, gsl::span<const AudioConfig::SampleFormat> samples)
{
	streamingClip->addInterleavedSamples(samples);	
}
