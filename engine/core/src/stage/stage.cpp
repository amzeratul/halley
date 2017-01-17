#include "stage/stage.h"

using namespace Halley;

Stage::Stage(String _name)
	: name(_name)
{
}

InputAPI& Stage::getInputAPI() const
{
	return *api->input;
}

VideoAPI& Stage::getVideoAPI() const
{
	return *api->video;
}

AudioAPI& Stage::getAudioAPI() const
{
	return *api->audio;
}

CoreAPI& Stage::getCoreAPI() const
{
	return *api->core;
}

Resources& Stage::getResources() const
{
	return api->core->getResources();
}

Game& Stage::getGame() const
{
	return *game;
}

void Stage::setGame(Game& g)
{
	game = &g;
}

void Stage::doInit(HalleyAPI* _api)
{
	api = _api;
	init();
}

void Stage::doDeInit()
{
	deInit();
}
