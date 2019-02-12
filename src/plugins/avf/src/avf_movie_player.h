#pragma once
#include "halley/core/api/movie_api.h"
#include "halley/resources/resource_data.h"
#include "halley/time/halleytime.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/sprite/sprite.h"
#include <halley/audio/audio_clip.h>
#include "halley/core/graphics/movie/movie_player.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

namespace Halley
{
	class VideoAPI;

	class AVFMoviePlayer : public MoviePlayer
	{
	public:
		AVFMoviePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data);
		~AVFMoviePlayer() noexcept;

	protected:
		void requestVideoFrame() override;
		void requestAudioFrame() override;
		void onReset() override;

	private:
		void init();
		void translateError(NSError* error);

		std::shared_ptr<ResourceDataStream> data;
		String filePath;

		AVAsset* asset = nil;
		AVAssetReader* assetReader = nil;
		AVAssetReaderVideoCompositionOutput* videoOut = nil;
		AVAssetReaderAudioMixOutput* audioOut = nil;
	};
}
