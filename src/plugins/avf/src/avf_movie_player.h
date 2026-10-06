#pragma once
#include "halley/api/movie_api.h"
#include "halley/resources/resource_data.h"
#include "halley/time/halleytime.h"
#include "halley/graphics/texture.h"
#include "halley/graphics/sprite/sprite.h"
#include <halley/audio/audio_clip.h>
#include "halley/graphics/movie/movie_player.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

namespace Halley
{
	class VideoAPI;
	class HalleyAPI;

	class AVFMoviePlayer : public MoviePlayer
	{
	public:
		AVFMoviePlayer(const HalleyAPI& halleyAPI, std::shared_ptr<ResourceDataStream> data);
		~AVFMoviePlayer() noexcept;

	protected:
		void requestVideoFrame() override;
		void requestAudioFrame() override;
		void onReset() override;

	private:
		struct PlaneData {
			std::byte* data;
			int stride;
			int height;
		};

		void init();
		void startReading();
		void translateError(NSError* error);

		void readVideoSample(Time time, PlaneData yPlane, PlaneData uvPlane);
		void readAudioSample(Time time, gsl::span<const std::byte> data);

		std::shared_ptr<ResourceDataStream> data;
		String filePath;
		bool startedReading = false;

		AVAsset* asset = nil;
		AVAssetReader* assetReader = nil;
		AVAssetReaderVideoCompositionOutput* videoOut = nil;
		AVAssetReaderAudioMixOutput* audioOut = nil;
	};
}
