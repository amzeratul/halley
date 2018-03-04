#include "mf_movie_api.h"
#include <Mfapi.h>
#include "mf_movie_player.h"
using namespace Halley;

#ifdef _MSC_VER
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "Mfuuid.lib")
#endif

MFMovieAPI::MFMovieAPI(SystemAPI& system)
	: system(system)
{
}

void MFMovieAPI::init()
{
	auto hr = MFStartup(MF_VERSION);	
	if (!SUCCEEDED(hr)) {
		throw Exception("Unable to initialise MediaFoundation");
	}
}

void MFMovieAPI::deInit()
{
	MFShutdown();
}

std::shared_ptr<MoviePlayer> MFMovieAPI::makePlayer(VideoAPI& video, AudioAPI& audio, std::shared_ptr<ResourceDataStream> data)
{
	return std::make_shared<MFMoviePlayer>(video, audio, data);
}
