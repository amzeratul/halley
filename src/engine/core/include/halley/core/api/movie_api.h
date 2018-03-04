#pragma once

#include <memory>

namespace Halley {
	class ResourceDataStream;
	class VideoAPI;
	class AudioAPI;
	class Resources;
	class MoviePlayer;

	class MovieAPI
	{
	public:
		virtual ~MovieAPI() = default;

		virtual std::shared_ptr<MoviePlayer> makePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data) = 0;
    };
}
