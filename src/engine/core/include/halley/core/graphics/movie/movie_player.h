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

	struct MoviePlayerAliveFlag
	{
		bool isAlive = true;
		mutable std::mutex mutex;
	};
	
	class MoviePlayer
	{
	public:
		MoviePlayer(VideoAPI& video, AudioAPI& audio);
		virtual ~MoviePlayer();

		void play();
		void pause();
		void reset();
		void stop();

		virtual bool hasError() const;

		virtual void update(Time t);
		void render(Resources& resources, RenderContext& rc);
		Sprite getSprite(Resources& resources);

		MoviePlayerState getState() const;
		Vector2i getSize() const;

	protected:
		virtual void requestVideoFrame() = 0;
		virtual void requestAudioFrame() = 0;
		virtual void onReset();
		virtual void onStartPlay();
		virtual void waitForVideoInfo();
		virtual bool needsYV12Conversion() const;
		virtual bool shouldRecycleTextures() const;
		virtual void onDoneUsingTexture(std::shared_ptr<Texture> texture);
		virtual Rect4i getCropRect() const;
		
		void setVideoSize(Vector2i size);

		VideoAPI& getVideoAPI() const;
		AudioAPI& getAudioAPI() const;

		void onVideoFrameAvailable(Time time, TextureDescriptor&& descriptor);
		void onVideoFrameAvailable(Time time, std::shared_ptr<Texture> texture);
		void onAudioFrameAvailable(Time time, gsl::span<const short> samples);
		void onAudioFrameAvailable(Time time, gsl::span<const float> samples);

		std::shared_ptr<MoviePlayerAliveFlag> getAliveFlag() const;

		std::vector<MoviePlayerStream> streams;
		std::list<std::shared_ptr<Texture>> recycleTexture;
		std::list<PendingFrame> pendingFrames;
		int maxVideoFrames;
		int maxAudioSamples;

	private:
		VideoAPI& video;
		AudioAPI& audio;

		MoviePlayerState state = MoviePlayerState::Uninitialised;

		Vector2i videoSize;
		std::shared_ptr<Texture> currentTexture;
		std::shared_ptr<TextureRenderTarget> renderTarget;
		std::shared_ptr<Texture> renderTexture;

		std::shared_ptr<StreamingAudioClip> streamingClip;
		std::shared_ptr<IAudioHandle> audioHandle;

		std::atomic<bool> threadRunning;
		std::atomic<bool> threadAborted;
		std::thread workerThread;
		std::shared_ptr<MoviePlayerAliveFlag> aliveFlag;

		Time time = 0;

		void startThread();
		void stopThread();
		void threadEntry();

		virtual bool useCustomThreads() const;
		virtual void stopCustomThreads();

		bool needsMoreVideoFrames() const;
		bool needsMoreAudioFrames() const;
	};
}

