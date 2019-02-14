#include "avf_movie_player.h"
#include "halley/maths/random.h"
#include "halley/support/logger.h"
#include "halley/core/graphics/texture_descriptor.h"
#include "halley/core/api/video_api.h"
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>

using namespace Halley;

AVFMoviePlayer::AVFMoviePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data)
	: MoviePlayer(video, audio)
	, data(std::move(data))
{
	init();
}

AVFMoviePlayer::~AVFMoviePlayer() noexcept
{
	reset();

	if (videoOut) {
		[videoOut release];
		videoOut = nil;
	}
	if (audioOut) {
		[audioOut release];
		audioOut = nil;
	}
	if (assetReader) {
		[assetReader cancelReading];
		[assetReader release];
		assetReader = nil;
	}
	if (asset) {
		[asset release];
		asset = nil;
	}

	if (!filePath.isEmpty()) {
		unlink(filePath.c_str());
	}
}

void AVFMoviePlayer::requestVideoFrame()
{
	startReading();

	CMSampleBufferRef sample = [videoOut copyNextSampleBuffer];
	if (sample == nil) {
		if ([assetReader status] == AVAssetReaderStatusFailed) {
			translateError([assetReader error]);
		} else {
			streams.at(0).eof = true;
		}
		return;
	}

	CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sample);
	if (pixelBuffer == nil) {
		CMSampleBufferInvalidate(sample);
		CFRelease(sample);
		throw Exception("Video sample did not contain an image buffer", HalleyExceptions::MoviePlugin);
	}

	CMTime timestamp = CMSampleBufferGetOutputPresentationTimeStamp(sample);
	Time sampleTime = CMTimeGetSeconds(timestamp);

	CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
	Expects(getSize() == Vector2i(CVPixelBufferGetWidth(pixelBuffer), CVPixelBufferGetHeight(pixelBuffer)));

	Expects(CVPixelBufferIsPlanar(pixelBuffer));
	PlaneData yPlane = {
		reinterpret_cast<gsl::byte*>(CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0)),
		int(CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0)),
		int(CVPixelBufferGetHeightOfPlane(pixelBuffer, 0))
	};
	PlaneData uvPlane = {
		reinterpret_cast<gsl::byte*>(CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1)),
		int(CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1)),
		int(CVPixelBufferGetHeightOfPlane(pixelBuffer, 1))
	};
	readVideoSample(sampleTime, std::move(yPlane), std::move(uvPlane));

	CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
	CMSampleBufferInvalidate(sample);
	CFRelease(sample);
}

void AVFMoviePlayer::requestAudioFrame()
{
	startReading();

	CMSampleBufferRef sample = [audioOut copyNextSampleBuffer];
	if (sample == nil) {
		if ([assetReader status] == AVAssetReaderStatusFailed) {
			translateError([assetReader error]);
		} else {
			streams.at(1).eof = true;
		}
		return;
	}

	CMTime timestamp = CMSampleBufferGetOutputPresentationTimeStamp(sample);
	Time sampleTime = CMTimeGetSeconds(timestamp);

	CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sample);
	if (blockBuffer == nil) {
		CMSampleBufferInvalidate(sample);
		CFRelease(sample);
		throw Exception("Audio sample did not contain a data buffer", HalleyExceptions::MoviePlugin);
	}

	size_t lengthAtOffset;
	size_t totalLength;
	char* data;
	auto error = CMBlockBufferGetDataPointer(blockBuffer, 0, &lengthAtOffset, &totalLength, &data);
	if (error != kCMBlockBufferNoErr) {
		throw Exception("Reading audio data failed; error: " + toString(int(error)), HalleyExceptions::MoviePlugin);
	}

	readAudioSample(sampleTime, gsl::as_bytes(gsl::span<char>(data, lengthAtOffset)));
}

void AVFMoviePlayer::onReset()
{
}

void AVFMoviePlayer::init()
{
	char tempPath[] = "/tmp/videoXXXXXX.mp4";
	int fd = mkstemps(tempPath, 4);
	if (fd == -1) {
		throw Exception("Unable to create temporary file for MoviePlayer", HalleyExceptions::MoviePlugin);
	}
	close(fd);
	filePath = tempPath;
	NSURL* fileUrl = [NSURL fileURLWithPath:[NSString stringWithUTF8String:tempPath]];

	auto reader = data->getReader();
	std::ofstream fs(filePath, std::ios::binary | std::ios::out);
	std::array<gsl::byte, 8192> buffer;
	int bytesRead = 0;
	do {
		bytesRead = reader->read(buffer);
		fs.write(reinterpret_cast<const char*>(buffer.data()), bytesRead);
	} while (bytesRead == buffer.size());
	fs.close();

	asset = [AVAsset assetWithURL:fileUrl];
	[asset retain];

	NSError* error = nil;
	assetReader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
	translateError(error);
	[assetReader retain];

	NSArray<AVAssetTrack*>* videoTracks = [asset tracksWithMediaType:AVMediaTypeVideo];
	videoOut = [[AVAssetReaderVideoCompositionOutput alloc] initWithVideoTracks:videoTracks videoSettings:nil];
	[videoOut setVideoComposition:[AVVideoComposition videoCompositionWithPropertiesOfAsset:asset]];
	[videoOut retain];

	NSArray<AVAssetTrack*>* audioTracks = [asset tracksWithMediaType:AVMediaTypeAudio];
	audioOut = [[AVAssetReaderAudioMixOutput alloc] initWithAudioTracks:audioTracks audioSettings:nil];
	[audioOut retain];

	[assetReader addOutput:videoOut];
	[assetReader addOutput:audioOut];

	CGSize naturalSize = [[videoTracks objectAtIndex:0] naturalSize];
	int width = (int)std::ceil(naturalSize.width);
	int height = (int)std::ceil(naturalSize.height);
	setVideoSize(Vector2i(width, height));

	streams.emplace_back(MoviePlayerStream { MoviePlayerStreamType::Video });
	streams.emplace_back(MoviePlayerStream { MoviePlayerStreamType::Audio });

	reset();
}

void AVFMoviePlayer::startReading()
{
	if (!startedReading) {
		startedReading = true;
		if (![assetReader startReading]) {
			translateError([assetReader error]);
		}
	}
}

void AVFMoviePlayer::translateError(NSError* error)
{
	if (error == nil) {
		return;
	}

	String description([[error localizedDescription] UTF8String]);
	int code = (int)[error code];
	throw Exception("Error while processing movie file (" + toString(code) + "): " + description, HalleyExceptions::MoviePlugin);
}

void AVFMoviePlayer::readVideoSample(Time time, PlaneData yPlane, PlaneData uvPlane)
{
	if (!yPlane.data || !uvPlane.data) {
		throw Exception("Null pointer provided to AVFMoviePlayer::readVideoSample.", HalleyExceptions::MoviePlugin);
	}
	if (yPlane.stride <= 0) {
		throw Exception("AVFMoviePlayer::readVideoSample, Y stride is not positive: " + toString(yPlane.stride), HalleyExceptions::MoviePlugin);
	}
	if (uvPlane.stride <= 0) {
		throw Exception("AVFMoviePlayer::readVideoSample, UV stride is not positive: " + toString(uvPlane.stride), HalleyExceptions::MoviePlugin);
	}
	if (yPlane.stride != uvPlane.stride) {
		throw Exception("AVFMoviePlayer::readVideoSample, UV stride does not match Y stride", HalleyExceptions::MoviePlugin);
	}

	const int stride = yPlane.stride;
	const auto videoSize = getSize();
	const int yPlaneHeight = alignUp(yPlane.height, 16);
	const int uvPlaneHeight = alignUp(uvPlane.height, 16);
	const int width = alignUp(videoSize.x, 16);
	const int height = yPlaneHeight + uvPlaneHeight;

	Bytes myData(stride * height);
	memcpy(myData.data(), yPlane.data, yPlane.height * stride);
	memcpy(myData.data() + yPlaneHeight * stride, uvPlane.data, uvPlane.height * stride);

	TextureDescriptor descriptor;
	descriptor.format = TextureFormat::Indexed;
	descriptor.pixelFormat = PixelDataFormat::Image;
	descriptor.size = Vector2i(width, height);
	descriptor.pixelData = TextureDescriptorImageData(std::move(myData), stride);

	onVideoFrameAvailable(time, std::move(descriptor));
}

void AVFMoviePlayer::readAudioSample(Time time, gsl::span<const gsl::byte> data)
{
	auto src = gsl::span<const short>(reinterpret_cast<const short*>(data.data()), data.size() / sizeof(short));

	std::vector<AudioConfig::SampleFormat> samples(src.size());
	for (int i = 0; i < src.size(); ++i) {
		samples[i] = src[i] / 32768.0f;
	}

	onAudioFrameAvailable(time, samples);
}
