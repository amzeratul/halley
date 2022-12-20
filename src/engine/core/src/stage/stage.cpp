#include "stage/stage.h"

#include "game/game.h"

using namespace Halley;

Stage::Stage(String name)
	: name(std::move(name))
{
}

void Stage::init()
{
}

void Stage::onStartFrame(Time dt)
{
}

void Stage::onFixedUpdate(Time dt)
{
}

void Stage::onVariableUpdate(Time dt)
{
}

void Stage::onRender(RenderContext& rc) const
{
}

void Stage::onStartFrame(Time dt, IFrameData& frameData)
{
	onStartFrame(dt);
}

void Stage::onFixedUpdate(Time dt, IFrameData& frameData)
{
	onFixedUpdate(dt);
}

void Stage::onVariableUpdate(Time dt, IFrameData& frameData)
{
	onVariableUpdate(dt);
}

void Stage::onRender(RenderContext& rc, IFrameData& frameData) const
{
	onRender(rc);
}

const HalleyAPI& Stage::getAPI() const
{
	return *api;
}

bool Stage::onQuitRequested()
{
	return true;
}

std::unique_ptr<IFrameData> Stage::makeFrameData()
{
	return game->makeFrameData();
}

bool Stage::hasMultithreadedRendering() const
{
	return false;
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
	Expects(resources);
	return *resources;
}

Game& Stage::getGame() const
{
	Expects(game);
	return *game;
}

void Stage::setGame(Game& g)
{
	game = &g;
}

void Stage::doInit(const HalleyAPI* _api, Resources& _resources)
{
	resources = &_resources;
	api = _api;
	init();
}
