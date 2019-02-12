#include "avf_movie_player.h"
#include <halley/maths/random.h>
#include <halley/support/logger.h>
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
	CMSampleBufferRef sample = [videoOut copyNextSampleBuffer];
	CVImageBufferRef image = CMSampleBufferGetImageBuffer(sample);
}

void AVFMoviePlayer::requestAudioFrame()
{
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

	if ([assetReader startReading] == NO) {
		translateError([assetReader error]);
	}

	reset();
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
