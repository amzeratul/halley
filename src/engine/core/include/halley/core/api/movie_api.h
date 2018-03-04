#pragma once

#include <halley/maths/vector2.h>
#include <halley/time/halleytime.h>
#include <halley/core/graphics/sprite/sprite.h>

namespace Halley {
	class ResourceDataStream;
	class VideoAPI;
	class AudioAPI;
	class Resources;

	enum class MoviePlayerState
	{
		Uninitialised,
		Loading,
		Paused,
		Playing,
		Finished
	};

	class MoviePlayer
	{
	public:
		virtual ~MoviePlayer() = default;

		virtual void play() = 0;
		virtual void pause() = 0;

		virtual void update(Time t) = 0;
		virtual Sprite getSprite(Resources& resources) = 0;

		virtual MoviePlayerState getState() const = 0;
		virtual Vector2i getSize() const = 0;
	};

	class MovieAPI
	{
	public:
		virtual ~MovieAPI() = default;

		virtual std::shared_ptr<MoviePlayer> makePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data) = 0;
    };
}
