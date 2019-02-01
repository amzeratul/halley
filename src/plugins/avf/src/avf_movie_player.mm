#include "avf_movie_player.h"
#include <halley/maths/random.h>
#include <halley/support/logger.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#import <AVFoundation/AVFoundation.h>

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
	if (!filePath.isEmpty()) {
		unlink(filePath.c_str());
	}
}

void AVFMoviePlayer::requestVideoFrame()
{
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

	AVAsset* asset = [AVAsset assetWithURL:fileUrl];
}
