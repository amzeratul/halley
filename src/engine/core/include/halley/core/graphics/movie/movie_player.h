#pragma once

#include <halley/maths/vector2.h>
#include <halley/time/halleytime.h>
#include <halley/core/graphics/sprite/sprite.h>
#include <gsl/gsl>
#include <thread>
#include <mutex>
#include <atomic>

namespace Halley
{
	class RenderContext;
	class TextureDescriptor;
	class VideoAPI;
	class AudioAPI;
	class StreamingAudioClip;
	class IAudioHandle;
	class TextureRenderTarget;

	enum class MoviePlayerState
	{
		Uninitialised,
		Loading,
		Paused,
		StartingToPlay,
		Playing,
		Finished
	};

	enum class MoviePlayerStreamType
	{
		Unknown,
		Video,
		Audio,
		Other
	};

	class MoviePlayerStream
	{
	public:
		MoviePlayerStreamType type = MoviePlayerStreamType::Unknown;
		bool playing = true;
		bool eof = false;
	};

	struct PendingFrame
	{
		std::shared_ptr<Texture> texture;
		Time time;
	};
	
	class MoviePlayer
	{
	public:
		MoviePlayer(VideoAPI& video, AudioAPI& audio);
		virtual ~MoviePlayer();

		void play();
		void pause();
		void reset();

		void update(Time t);
		void render(Resources& resources, RenderContext& rc);
		Sprite getSprite(Resources& resources);

		MoviePlayerState getState() const;
		Vector2i getSize() const;

	protected:
		virtual void requestVideoFrame() = 0;
		virtual void requestAudioFrame() = 0;
		virtual void onReset() {}
		
		void setVideoSize(Vector2i size);

		VideoAPI& getVideoAPI() const;
		AudioAPI& getAudioAPI() const;

		void onVideoFrameAvailable(Time time, TextureDescriptor&& descriptor);
		void onVideoFrameAvailable(Time time, std::shared_ptr<Texture> texture);
		void onAudioFrameAvailable(Time time, gsl::span<const short> samples);
		void onAudioFrameAvailable(Time time, gsl::span<const float> samples);

		std::vector<MoviePlayerStream> streams;

	private:
		VideoAPI& video;
		AudioAPI& audio;

		MoviePlayerState state = MoviePlayerState::Uninitialised;

		Vector2i videoSize;
		std::shared_ptr<Texture> currentTexture;
		std::list<PendingFrame> pendingFrames;
		std::list<std::shared_ptr<Texture>> recycleTexture;
		std::shared_ptr<TextureRenderTarget> renderTarget;
		std::shared_ptr<Texture> renderTexture;

		std::shared_ptr<StreamingAudioClip> streamingClip;
		std::shared_ptr<IAudioHandle> audioHandle;

		std::atomic<bool> threadRunning;
		std::thread workerThread;
		mutable std::mutex mutex;

		Time time = 0;

		void startThread();
		void stopThread();
		void threadEntry();

		bool needsMoreVideoFrames() const;
		bool needsMoreAudioFrames() const;
	};
}

