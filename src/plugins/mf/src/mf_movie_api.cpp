#include "mf_movie_api.h"
#include <Mfapi.h>
using namespace Halley;

#ifdef _MSC_VER
#pragma comment(lib, "Mfplat.lib")
#endif

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
