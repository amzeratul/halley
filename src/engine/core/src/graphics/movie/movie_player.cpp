#include "halley/core/graphics/movie/movie_player.h"
#include "resources/resources.h"
#include "halley/concurrency/concurrent.h"
#include "halley/core/graphics/texture_descriptor.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/render_target/render_target_texture.h"
#include "halley/core/api/video_api.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/render_context.h"
#include "halley/audio/audio_clip.h"
#include <chrono>
#include "halley/audio/audio_position.h"
#include "halley/support/logger.h"

using namespace Halley;
using namespace std::chrono_literals;

MoviePlayer::MoviePlayer(VideoAPI& video, AudioAPI& audio)
	: video(video)
	, audio(audio)
	, threadRunning(false)
	, maxVideoFrames(7)
	, maxAudioSamples(20000)
	, aliveFlag(std::make_shared<MoviePlayerAliveFlag>())
{
}

MoviePlayer::~MoviePlayer()
{
	{
		std::shared_ptr<MoviePlayerAliveFlag> alive = getAliveFlag();
		std::unique_lock<std::mutex> lock(alive->mutex);
		alive->isAlive = false;
	}
	stopThread();
}

void MoviePlayer::play()
{
	if (state == MoviePlayerState::Paused) {
		startThread();

		if (!streamingClip) {
			streamingClip = std::make_shared<StreamingAudioClip>(2);
		}

		state = MoviePlayerState::StartingToPlay;

		for (auto& s: streams) {
			s.playing = true;
		}

		onStartPlay();
		waitForVideoInfo();

		if (needsYV12Conversion()) {
			renderTexture = video.createTexture(getSize());
			auto desc = TextureDescriptor(getSize(), TextureFormat::RGBA);
			desc.useFiltering = true;
			desc.isRenderTarget = true;
			renderTexture->load(std::move(desc));

			renderTarget = video.createTextureRenderTarget();
			renderTarget->setTarget(0, renderTexture);
			renderTarget->setViewPort(Rect4i(Vector2i(), getSize()));
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

		audioHandle->stop();
	}
}

void MoviePlayer::reset()
{
	stopThread();

	state = MoviePlayerState::Paused;
	time = 0;

	currentTexture.reset();
	pendingFrames.clear();
	streamingClip.reset();
	if (audioHandle) {
		audioHandle->stop();
		audioHandle.reset();
	}

	onReset();
}

void MoviePlayer::stop()
{
	pause();
	reset();
	state = MoviePlayerState::Finished;
}

bool MoviePlayer::hasError() const
{
	return false;
}

void MoviePlayer::update(Time t)
{
	if (threadRunning && threadAborted) {
		reset();
	}

	if (state == MoviePlayerState::Playing) {
		time += t;
	}

	if (state == MoviePlayerState::Playing || state == MoviePlayerState::StartingToPlay) {
		{
			std::shared_ptr<MoviePlayerAliveFlag> alive = getAliveFlag();
			std::unique_lock<std::mutex> lock(alive->mutex);
			if (!pendingFrames.empty()) {
				auto& next = pendingFrames.front();
				if (time >= next.time) {
					if (currentTexture) {
						if (shouldRecycleTextures()) {
							recycleTexture.push_back(currentTexture);
						}
						onDoneUsingTexture(currentTexture);
					}
					currentTexture = next.texture;
					pendingFrames.pop_front();
				}
			}
		}

		if (state == MoviePlayerState::Playing && pendingFrames.empty() && !needsMoreVideoFrames()) {
			state = MoviePlayerState::Finished;
		}

		if (state == MoviePlayerState::StartingToPlay) {
			if (pendingFrames.size() >= 3) {
				if (streamingClip) {
					audioHandle = audio.play(streamingClip, AudioPosition::makeUI(), 0.5f);
				}
				state = MoviePlayerState::Playing;
			}
		}
	}

}

void MoviePlayer::render(Resources& resources, RenderContext& rc)
{
	if (needsYV12Conversion() && currentTexture) {
		Camera cam;
		cam.setPosition(Vector2f(videoSize) * 0.5f);
		cam.setZoom(1.0f);

		auto c = rc.with(*renderTarget).with(cam);
		c.bind([&] (Painter& painter)
		{
			auto matDef = resources.get<MaterialDefinition>("Halley/NV12Video");
			Sprite().setImage(currentTexture, matDef).setTexRect(Rect4f(0, 0, 1, 1)).setSize(Vector2f(videoSize)).draw(painter);
		});
		currentTexture.reset();
	}
}

Sprite MoviePlayer::getSprite(Resources& resources)
{
	Rect4i crop = getCropRect();
	Rect4f texRect = Rect4f(crop) / Vector2f(videoSize);

	if (needsYV12Conversion()) {
		if (renderTexture) {
			auto matDef = resources.get<MaterialDefinition>("Halley/Sprite");
			return Sprite().setImage(renderTexture, matDef).setTexRect(texRect).setSize(Vector2f(videoSize));
		} else {
			return Sprite().setMaterial(resources, "Halley/SolidColour").setColour(Colour4f(0, 0, 0)).setSize(Vector2f(videoSize));
		}
	} else {
		if (currentTexture) {
			auto matDef = resources.get<MaterialDefinition>("Halley/Sprite");
			return Sprite().setImage(currentTexture, matDef).setTexRect(texRect).setSize(Vector2f(videoSize));
		} else {
			return Sprite().setMaterial(resources, "Halley/SolidColour").setColour(Colour4f(0, 0, 0)).setSize(Vector2f(videoSize));
		}
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

void MoviePlayer::startThread()
{
	if (!threadRunning) {
		threadRunning = true;
		threadAborted = false;
		workerThread = std::thread([=] () {
			try {
				threadEntry();
			} catch (Exception& e) {
				Logger::logException(e);
				threadAborted = true;
			}
		});
	}
}

void MoviePlayer::stopThread()
{
	if (threadRunning) {
		threadRunning = false;
		workerThread.join();
	}
}

void MoviePlayer::threadEntry()
{
	while (threadRunning) {
		bool hadWork = false;
		if (needsMoreAudioFrames()) {
			requestAudioFrame();
			hadWork = true;
		} 
		if (needsMoreVideoFrames()) {
			requestVideoFrame();
			hadWork = true;
		}
		
		if (!hadWork) {
			std::this_thread::sleep_for(2ms);
		}
	}
}

bool MoviePlayer::needsMoreVideoFrames() const
{
	std::shared_ptr<MoviePlayerAliveFlag> alive = getAliveFlag();
	std::unique_lock<std::mutex> lock(alive->mutex);

	if (state != MoviePlayerState::Playing && state != MoviePlayerState::StartingToPlay) {
		return false;
	}

	const MoviePlayerStream* videoStream = nullptr;
	for (auto& s: streams) {
		if (s.type == MoviePlayerStreamType::Video) {
			videoStream = &s;
			break;
		}
	}

	if (!videoStream) {
		return false;
	}

	return int(pendingFrames.size()) < maxVideoFrames && !videoStream->eof;
}

bool MoviePlayer::needsMoreAudioFrames() const
{
	std::shared_ptr<MoviePlayerAliveFlag> alive = getAliveFlag();
	std::unique_lock<std::mutex> lock(alive->mutex);

	if (state != MoviePlayerState::Playing && state != MoviePlayerState::StartingToPlay) {
		return false;
	}

	const MoviePlayerStream* audioStream = nullptr;
	for (auto& s: streams) {
		if (s.type == MoviePlayerStreamType::Audio) {
			audioStream = &s;
		}
	}

	if (!audioStream) {
		return false;
	}

	if (!streamingClip) {
		return false;
	}

	return int(streamingClip->getSamplesLeft()) < maxAudioSamples && !audioStream->eof;
}

void MoviePlayer::onReset()
{
}

void MoviePlayer::onStartPlay()
{
}

void MoviePlayer::waitForVideoInfo()
{
}

bool MoviePlayer::needsYV12Conversion() const
{
	return true;
}

bool MoviePlayer::shouldRecycleTextures() const
{
	return false;
}

void MoviePlayer::onDoneUsingTexture(std::shared_ptr<Texture> texture)
{
}

Rect4i MoviePlayer::getCropRect() const
{
	return Rect4i(0, 0, videoSize.x, videoSize.y);
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
	auto desc = std::make_shared<TextureDescriptor>(std::move(descriptor));
	auto alive = getAliveFlag();

	Concurrent::execute(Executors::getVideoAux(), [this, time, alive, desc = std::move(desc)] ()
	{
		auto descriptor = TextureDescriptor(std::move(*desc));
		std::shared_ptr<Texture> tex;

		{
			std::unique_lock<std::mutex> lock(alive->mutex);

			if (alive->isAlive) {
				if (recycleTexture.empty()) {
					tex = video.createTexture(descriptor.size);
				}
				else {
					tex = recycleTexture.front();
					recycleTexture.pop_front();
				}

				const auto iter = std::find_if(pendingFrames.begin(), pendingFrames.end(), [=] (const PendingFrame& f)
				{
					return f.time > time;
				});
				pendingFrames.insert(iter, { tex, time });
			}
		}

		if (tex) {
			tex->load(std::move(descriptor));
		}
	});
}

void MoviePlayer::onVideoFrameAvailable(Time time, std::shared_ptr<Texture> texture)
{
	std::shared_ptr<MoviePlayerAliveFlag> alive = getAliveFlag();
	std::unique_lock<std::mutex> lock(alive->mutex);
	pendingFrames.push_back({texture, time});
}

void MoviePlayer::onAudioFrameAvailable(Time time, gsl::span<const short> origSamples)
{
	std::vector<AudioConfig::SampleFormat> samples(origSamples.size());
	for (size_t i = 0; i < size_t(origSamples.size()); ++i) {
		samples[i] = origSamples[i] / 32768.0f;
	}
	onAudioFrameAvailable(time, gsl::span<const AudioConfig::SampleFormat>(samples.data(), samples.size()));
}

void MoviePlayer::onAudioFrameAvailable(Time time, gsl::span<const AudioConfig::SampleFormat> samples)
{
	streamingClip->addInterleavedSamples(samples);	
}

std::shared_ptr<MoviePlayerAliveFlag> MoviePlayer::getAliveFlag() const
{
	return aliveFlag;
}
