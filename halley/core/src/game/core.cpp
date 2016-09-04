#include <iostream>
#include "halley/core/game/core.h"
#include "halley/core/game/game.h"
#include "halley/core/game/environment.h"
#include "api/halley_api.h"
#include "graphics/camera.h"
#include "graphics/render_context.h"
#include "graphics/render_target/render_target_screen.h"
#include "graphics/window.h"
#include "resources/resources.h"
#include "resources/resource_locator.h"
#include "resources/standard_resources.h"
#include <halley/os/os.h>
#include <halley/support/debug.h>
#include <halley/support/console.h>
#include <halley/concurrency/concurrent.h>
#include <fstream>

#pragma warning(disable: 4996)

using namespace Halley;

Core::Core(std::unique_ptr<Game> g, Vector<std::string> _args)
{
	statics.setup();

	game = std::move(g);

	// Set paths
	environment = std::make_unique<Environment>();
	if (_args.size() > 0) {
		environment->parseProgramPath(_args[0]);
		args.resize(_args.size() - 1);
		std::copy(_args.begin() + 1, _args.end(), args.begin());
	}
	environment->setDataPath(game->getDataPath());

	// Basic initialization
	game->init(*environment, args);

	// Console
	if (game->shouldCreateSeparateConsole()) {
		hasConsole = true;
		OS::get().createLogConsole(game->getName());
	}
	OS::get().initializeConsole();
	setOutRedirect(false);

	std::cout << ConsoleColour(Console::GREEN) << "Halley is initializing..." << ConsoleColour() << std::endl;

	// Debugging initialization
	Debug::setErrorHandling();
	Concurrent::setThreadName("main");

	// Seed RNG
	time_t curTime = time(nullptr);
	clock_t curClock = clock();
	int seed = static_cast<int>(curTime) ^ static_cast<int>(curClock) ^ 0x3F29AB51;
	srand(seed);

	// Info
	std::cout << "Data path is " << ConsoleColour(Console::DARK_GREY) << environment->getDataPath() << ConsoleColour() << std::endl;
}

Core::~Core()
{
	deInit();
}

void Core::onSuspended()
{
	if (api->videoInternal) {
		api->videoInternal->onSuspend();
	}
	if (api->inputInternal) {
		api->inputInternal->onSuspend();
	}

	std::cout.flush();
	out.reset();
}

void Core::onReloaded()
{
	if (game->shouldCreateSeparateConsole()) {
		setOutRedirect(true);
	}
	statics.setup();

	if (api->inputInternal) {
		api->inputInternal->onResume();
	}
	if (api->videoInternal) {
		api->videoInternal->onResume();
	}
}

void Core::onTerminatedInError()
{
	hasError = true;
}

void Core::init()
{
	statics.setup();

	// Computer info
#ifndef _DEBUG
	showComputerInfo();
#endif

	// Initialize game
	api = HalleyAPI::create(this, game->initPlugins(*this));

	// Resources
	initResources();

	// Start game
	setStage(game->startGame(&*api));
	
	// Get video resources
	if (api->video) {
		painter = std::move(api->videoInternal->makePainter());
	}
}

void Core::deInit()
{
	std::cout << "Game shutting down." << std::endl;

	// Ensure stage is cleaned up
	transitionStage();

	// Deinit game
	game->endGame();

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

#ifdef _WIN32
	if (hasError && hasConsole) {
		system("pause");
	}
#endif
}

void Core::initResources()
{
	auto locator = std::make_unique<ResourceLocator>();
	game->initResourceLocator(environment->getProgramPath() + "/", *locator);
	resources = std::make_unique<Resources>(std::move(locator), &*api);
	StandardResources::initialize(*resources);
}

void Core::setOutRedirect(bool appendToExisting)
{
	auto outStream = std::make_shared<std::ofstream>(environment->getDataPath() + "log.txt", appendToExisting ? std::ofstream::app : std::ofstream::trunc);
	out = std::make_unique<RedirectStreamToStream>(std::cout, outStream, false);
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
	if (api->video) {
		auto windowSize = api->video->getWindow().getSize();
		screenTarget = std::make_unique<ScreenRenderTarget>(Rect4i(Vector2i(), windowSize));
		camera = std::make_unique<Camera>(Vector2f(windowSize) * 0.5f, Vector2f(windowSize));
	}

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
	std::cout << "\tOS:   " << ConsoleColour(Console::DARK_GREY) << computerData.osName << ConsoleColour() << "\n";
	std::cout << "\tCPU:  " << ConsoleColour(Console::DARK_GREY) << computerData.cpuName << ConsoleColour() << "\n";
	std::cout << "\tGPU:  " << ConsoleColour(Console::DARK_GREY) << computerData.gpuName << ConsoleColour() << "\n";
	std::cout << "\tRAM:  " << ConsoleColour(Console::DARK_GREY) << String::prettySize(computerData.RAM) << ConsoleColour() << "\n";
	std::cout << "\tTime: " << ConsoleColour(Console::DARK_GREY) << curTime << ConsoleColour() << "\n" << std::endl;
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

Vector<Plugin*> Core::getPlugins(PluginType type)
{
	Vector<Plugin*> result;
	for (auto& p : plugins[type]) {
		result.push_back(&*p);
	}
	return result;
}
