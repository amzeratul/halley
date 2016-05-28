#include <iostream>
#include <fstream>
#include "halley/core/game/core.h"
#include "halley/core/game/game.h"
#include "halley/core/game/environment.h"
#include "api/halley_api.h"
#include "graphics/camera.h"
#include "graphics/render_context.h"
#include "graphics/render_target/render_target_screen.h"
#include "resources/resources.h"
#include "resources/resource_locator.h"
#include "resources/standard_resources.h"
#include <halley/os/os.h>
#include <halley/concurrency/thread_pool.h>
#include <halley/support/debug.h>
#include <halley/support/console.h>

#pragma warning(disable: 4996)

using namespace Halley;

Core::Core(std::unique_ptr<Game> g, std::vector<String> args)
{
	statics.setup();

	game = std::move(g);

	// Set paths
	environment = std::make_unique<Environment>();
	if (args.size() > 0) {
		environment->parseProgramPath(args[0]);
	}
	environment->setDataPath(game->getDataPath());

	// Console
	if (game->isDevBuild()) {
		OS::get().createLogConsole(game->getName());
	}

	std::cout << ConsoleColor(Console::GREEN) << "Halley is initializing..." << ConsoleColor() << std::endl;

	// Debugging initialization
	Debug::setErrorHandling();
	Concurrent::setThreadName("main");

	// Seed RNG
	time_t curTime = time(nullptr);
	clock_t curClock = clock();
	int seed = static_cast<int>(curTime) ^ static_cast<int>(curClock) ^ 0x3F29AB51;
	srand(seed);

	// Redirect output
	auto outStream = std::make_shared<std::ofstream>(environment->getDataPath() + "log.txt", std::ios::out);
	out = std::make_unique<RedirectStreamToStream>(std::cout, outStream, false);
	std::cout << "Data path is " << ConsoleColor(Console::DARK_GREY) << environment->getDataPath() << ConsoleColor() << std::endl;
}

Core::~Core()
{
	deInit();
}

void Core::onReloaded()
{
	if (api->videoInternal) {
		api->videoInternal->reload();
	}
}

void Core::init()
{
	// Computer info
#ifndef _DEBUG
	showComputerInfo();
#endif

	// API
	api = HalleyAPI::create(this, game->initPlugins(*this));

	// Resources
	initResources();

	// Init game
	game->init(&*api);

	// Create frame
	setStage(game->makeStage(game->getInitialStage()));

	// Get video resources
	if (api->video) {
		painter = std::move(api->videoInternal->makePainter());
		screenTarget = std::make_unique<ScreenRenderTarget>(Rect4i(Vector2i(), api->video->getWindowSize()));
		camera = std::make_unique<Camera>(Vector2f(640, 360), Vector2f(1280, 720));
	}
}

void Core::deInit()
{
	std::cout << "Game shutting down." << std::endl;

	// Ensure stage is cleaned up
	transitionStage();

	// Deinit game
	game->deInit();

	// Deinit painter
	painter.reset();

	// Deinit resources
	resources.reset();
	
	// Deinit API
	api.reset();

	// Deinit console redirector
	std::cout << "Goodbye!" << std::endl;
	std::cout.flush();
	out.reset();
}

void Core::initResources()
{
	auto locator = std::make_unique<ResourceLocator>();
	game->initResourceLocator(environment->getProgramPath(), *locator);
	resources = std::make_unique<Resources>(std::move(locator), &*api);
	StandardResources::initialize(*resources);
}

void Core::pumpEvents(Time time)
{
	auto video = dynamic_cast<VideoAPIInternal*>(&*api->video);
	auto input = dynamic_cast<InputAPIInternal*>(&*api->input);
	input->beginEvents(time);
	running = api->system->generateEvents(video, input);
}

void Core::onFixedUpdate(Time time)
{
	if (isRunning()) {
		doFixedUpdate(time);
	}
}

void Core::onVariableUpdate(Time time)
{
	if (isRunning()) {
		doVariableUpdate(time);
	}

	if (isRunning()) {
		doRender(time);
	}
}

void Core::doFixedUpdate(Time time)
{
	auto& t = timers[int(TimeLine::FixedUpdate)];
	t.beginSample();

	pumpEvents(time);
	if (running) {
		if (currentStage) {
			currentStage->onFixedUpdate(time);
		}
	}

	t.endSample();
}

void Core::doVariableUpdate(Time time)
{
	auto& t = timers[int(TimeLine::VariableUpdate)];
	t.beginSample();

	if (running) {
		if (currentStage) {
			currentStage->onVariableUpdate(time);
		}
	}

	t.endSample();
}

void Core::doRender(Time)
{
	auto& t = timers[int(TimeLine::Render)];
	t.beginSample();

	if (api->video) {
		api->video->startRender();
		painter->startRender();

		if (currentStage) {
			RenderContext context(*painter, *camera, *screenTarget, Rect4i(screenTarget->getViewPort()));
			currentStage->onRender(context);
		}

		painter->endRender();
		api->video->finishRender();
	}

	t.endSample();
}

void Core::showComputerInfo() const
{
	time_t rawtime;
	time(&rawtime);
	String curTime = asctime(localtime(&rawtime));
	curTime.trim(true);

	auto computerData = OS::get().getComputerData();
	std::cout << "Computer data:" << "\n";
	//std::cout << "\tName: " << computerData.computerName << "\n";
	//std::cout << "\tUser: " << computerData.userName << "\n";
	std::cout << "\tOS:   " << ConsoleColor(Console::DARK_GREY) << computerData.osName << ConsoleColor() << "\n";
	std::cout << "\tCPU:  " << ConsoleColor(Console::DARK_GREY) << computerData.cpuName << ConsoleColor() << "\n";
	std::cout << "\tGPU:  " << ConsoleColor(Console::DARK_GREY) << computerData.gpuName << ConsoleColor() << "\n";
	std::cout << "\tRAM:  " << ConsoleColor(Console::DARK_GREY) << String::prettySize(computerData.RAM) << ConsoleColor() << "\n";
	std::cout << "\tTime: " << ConsoleColor(Console::DARK_GREY) << curTime << ConsoleColor() << "\n" << std::endl;
}

void Core::setStage(StageID stage)
{
	setStage(game->makeStage(stage));
}

void Core::setStage(std::unique_ptr<Stage> next)
{
	nextStage = std::move(next);
	pendingStageTransition = true;
}

void Core::quit()
{
	std::cout << "Game terminating via CoreAPI::quit()." << std::endl;
	running = false;
}

Resources& Core::getResources()
{
	return *resources;
}

long long Core::getAverageTime(TimeLine tl) const
{
	return timers[int(tl)].averageElapsedNanoSeconds();
}

long long Core::getElapsedTime(TimeLine tl) const
{
	return timers[int(tl)].lastElapsedNanoSeconds();
}

bool Core::transitionStage()
{
	// If it's not running anymore, reset stage
	if (!running && currentStage) {
		pendingStageTransition = true;
		nextStage.reset();
	}

	// Check if there's a stage waiting to be switched to
	if (pendingStageTransition) {
		// Get rid of current stage
		if (currentStage) {
			currentStage->deInit();
			currentStage.reset();
		}

		// Update stage
		currentStage = std::move(nextStage);

		// Prepare next stage
		if (currentStage) {
			currentStage->api = &*api;
			currentStage->init();
		} else {
			running = false;
		}

		pendingStageTransition = false;
		return true;
	} else {
		return false;
	}
}

void Core::registerPlugin(std::unique_ptr<Plugin> plugin)
{
	plugins[plugin->getType()].emplace_back(std::move(plugin));
}

std::vector<Plugin*> Core::getPlugins(PluginType type)
{
	std::vector<Plugin*> result;
	for (auto& p : plugins[type]) {
		result.push_back(&*p);
	}
	return result;
}
