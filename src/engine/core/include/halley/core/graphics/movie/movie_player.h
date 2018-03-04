#pragma once

#include <halley/maths/vector2.h>
#include <halley/time/halleytime.h>
#include <halley/core/graphics/sprite/sprite.h>
#include <gsl/gsl>

namespace Halley
{
	class TextureDescriptor;
	class VideoAPI;
	class AudioAPI;
	class StreamingAudioClip;

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
		virtual ~MoviePlayer() = default;

		void play();
		void pause();
		void reset();

		void update(Time t);
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

		std::shared_ptr<StreamingAudioClip> streamingClip;

		Time time = 0;
	};
}
