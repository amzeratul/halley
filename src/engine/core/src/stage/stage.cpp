#include "halley/stage/stage.h"

#include "halley/game/game.h"

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

void Stage::onStartFrame(Time dt, BaseFrameData& frameData)
{
	onStartFrame(dt);
}

void Stage::onFixedUpdate(Time dt, BaseFrameData& frameData)
{
	onFixedUpdate(dt);
}

void Stage::onVariableUpdate(Time dt, BaseFrameData& frameData)
{
	onVariableUpdate(dt);
}

void Stage::onRender(RenderContext& rc, BaseFrameData& frameData) const
{
	onRender(rc);
}

bool Stage::canRender() const
{
	return true;
}

const HalleyAPI& Stage::getAPI() const
{
	return *api;
}

bool Stage::onQuitRequested()
{
	return true;
}

std::unique_ptr<BaseFrameData> Stage::makeFrameData()
{
	return game->makeFrameData();
}

bool Stage::hasMultithreadedRendering() const
{
	return false;
}

InputAPI& Stage::getInputAPI() const
{
	HalleyAssertDev(api->input);
	return *api->input;
}

VideoAPI& Stage::getVideoAPI() const
{
	HalleyAssertDev(api->video);
	return *api->video;
}

AudioAPI& Stage::getAudioAPI() const
{
	HalleyAssertDev(api->audio);
	return *api->audio;
}

CoreAPI& Stage::getCoreAPI() const
{
	HalleyAssertDev(api->core);
	return *api->core;
}

SystemAPI& Stage::getSystemAPI() const
{
	HalleyAssertDev(api->system);
	return *api->system;
}

NetworkAPI& Stage::getNetworkAPI() const
{
	HalleyAssertDev(api->network);
	return *api->network;
}

MovieAPI& Stage::getMovieAPI() const
{
	HalleyAssertDev(api->movie);
	return *api->movie;
}

AnalyticsAPI& Stage::getAnalyticsAPI() const
{
	HalleyAssertDev(api->analytics);
	return *api->analytics;
}

WebAPI& Stage::getWebAPI() const
{
	HalleyAssertDev(api->web);
	return *api->web;
}

Resources& Stage::getResources() const
{
	HalleyAssertDev(resources);
	return *resources;
}

PlatformAPI& Stage::getPlatformAPI() const
{
	HalleyAssertDev(api->platform);
	return *api->platform;
}

Game& Stage::getGame() const
{
	HalleyAssertDev(game);
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


