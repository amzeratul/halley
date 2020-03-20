#include "stage/stage.h"

using namespace Halley;

Stage::Stage(String _name)
	: name(_name)
{
}

InputAPI& Stage::getInputAPI() const
{
	Expects(api->input);
	return *api->input;
}

VideoAPI& Stage::getVideoAPI() const
{
	Expects(api->video);
	return *api->video;
}

AudioAPI& Stage::getAudioAPI() const
{
	Expects(api->audio);
	return *api->audio;
}

CoreAPI& Stage::getCoreAPI() const
{
	Expects(api->core);
	return *api->core;
}

SystemAPI& Stage::getSystemAPI() const
{
	Expects(api->system);
	return *api->system;
}

NetworkAPI& Stage::getNetworkAPI() const
{
	Expects(api->network);
	return *api->network;
}

MovieAPI& Stage::getMovieAPI() const
{
	Expects(api->movie);
	return *api->movie;
}

Resources& Stage::getResources() const
{
	return api->core->getResources();
}

Game& Stage::getGame() const
{
	Expects(game != nullptr);
	return *game;
}

void Stage::setGame(Game& g)
{
	game = &g;
}

void Stage::doInit(const HalleyAPI* _api)
{
	api = _api;
	init();
}
