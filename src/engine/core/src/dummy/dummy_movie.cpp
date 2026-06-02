#include "dummy_movie.h"
using namespace Halley;

void DummyMovieAPI::init()
{
}

void DummyMovieAPI::deInit()
{
}

std::shared_ptr<MoviePlayer> DummyMovieAPI::makePlayer(const HalleyAPI& halleyAPI, std::shared_ptr<ResourceDataStream> data)
{
	return {};
}
