#include <iostream>
#include <fstream>
#include "core_runner.h"
#include "game.h"
#include "environment.h"
#include "../api/halley_api.h"
#include "../prec.h"
#include "../graphics/camera.h"
#include "../graphics/render_context.h"
#include "../graphics/render_target/render_target_screen.h"
#include "../resources/resources.h"
#include "../resources/resource_locator.h"
#include "../resources/standard_resources.h"

#pragma warning(disable: 4996)

using namespace Halley;

CoreRunner::CoreRunner(std::unique_ptr<Game> game, std::vector<String> args)
{
	run(std::move(game), args);
}

CoreRunner::~CoreRunner()
{
}

int CoreRunner::run(std::unique_ptr<Game> g, std::vector<String> args)
{
	game = std::move(g);
	
	bool initing = true;
	try {
		// Set paths
		if (args.size() > 0) {
			Environment::parseProgramPath(args[0]);
		}
		Environment::setDataPath(game->getDataPath());

		// Initialize
		init(StringArray(args.begin() + 1, args.end()));
		initing = false;
		runMainLoop(true, 60);
	}
	catch (std::exception& e) {
		handleException(e);
		return 1;
	}
	catch (...) {
		if (initing) std::cout << ConsoleColor(Console::RED) << "Unknown unhandled exception in initialization" << ConsoleColor() << std::endl;
		else std::cout << ConsoleColor(Console::RED) << "Unknown unhandled exception in main loop" << ConsoleColor() << std::endl;
		crashed = true;
		return 1;
	}

	try {
		std::cout << "Game shutting down." << std::endl;
		deInit();
		return 0;
	}
	catch (std::exception& e) {
		handleException(e);
		return 1;
	}
	catch (...) {
		std::cout << ConsoleColor(Console::RED) << "Unknown unhandled exception in deinitialization" << ConsoleColor() << std::endl;
		crashed = true;
		return 1;
	}
}

void CoreRunner::init(std::vector<String> args)
{
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
	auto outStream = std::make_shared<std::ofstream>(Environment::getDataPath() + "log.txt", std::ios::out);
	out = std::make_unique<RedirectStreamToStream>(std::cout, outStream, false);
	std::cout << "Data path is " << ConsoleColor(Console::DARK_GREY) << Environment::getDataPath() << ConsoleColor() << std::endl;

	// Computer info
#ifndef _DEBUG
	showComputerInfo();
#endif

	// API
	api = HalleyAPI::create(this, game->initPlugins());

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

void CoreRunner::deInit()
{
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

#ifdef _WIN32
	if (crashed) {
		system("pause");
	}
#endif
}

void CoreRunner::initResources()
{
	auto locator = std::make_unique<ResourceLocator>();
	game->initResourceLocator(*locator);
	resources = std::make_unique<Resources>(std::move(locator), &*api);
	StandardResources::initialize(*resources);
}

void CoreRunner::pumpEvents(Time time)
{
	auto video = dynamic_cast<VideoAPIInternal*>(&*api->video);
	auto input = dynamic_cast<InputAPIInternal*>(&*api->input);
	input->beginEvents(time);
	running = api->system->generateEvents(video, input);
}

void CoreRunner::onFixedUpdate(Time time)
{
	pumpEvents(time);
	if (running) {
		if (currentStage) {
			currentStage->onFixedUpdate(time);
		}
	}
}

void CoreRunner::onVariableUpdate(Time time)
{
	//pumpEvents();
	if (running) {
		if (currentStage) {
			currentStage->onVariableUpdate(time);
		}
	}
}

void CoreRunner::onRender(Time)
{
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
}

void CoreRunner::handleException(std::exception& e)
{
	std::cout << ConsoleColor(Console::RED) << "\n\nUnhandled exception: " << ConsoleColor(Console::DARK_RED) << e.what() << ConsoleColor() << std::endl;
	crashed = true;
}

void CoreRunner::showComputerInfo() const
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

void CoreRunner::setStage(StageID stage)
{
	setStage(game->makeStage(stage));
}

void CoreRunner::setStage(std::unique_ptr<Stage> next)
{
	nextStage = std::move(next);
	pendingStageTransition = true;
}

void CoreRunner::quit()
{
	std::cout << "Game terminating via CoreAPI::quit()." << std::endl;
	running = false;
}

Resources& CoreRunner::getResources()
{
	return *resources;
}

void CoreRunner::transitionStage()
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
	}
}

void CoreRunner::runMainLoop(bool capFrameRate, int fps)
{
	running = true;
	transitionStage();

	std::cout << ConsoleColor(Console::GREEN) << "\nStarting main loop." << ConsoleColor() << std::endl;
	Debug::trace("Game::runMainLoop begin");

	using Uint32 = unsigned int;

	// Set up the counters
	if (api->video) {
		api->video->flip();
	}
	Uint32 startTime = api->system->getTicks();
	Uint32 targetTime = startTime;
	Uint32 nSteps = 0;

	while (running) {
		Time delta = 1.0 / fps;

		if (delay > 0) {
			startTime += delay;
			targetTime += delay;
			delay = 0;
		}
		Uint32 curTime = api->system->getTicks();

		// Got anything to do?
		if (curTime >= targetTime) {
			// Step until we're up-to-date
			for (int i = 0; i < 10 && curTime >= targetTime; i++) {
				// Fixed step
				if (running) {
					onFixedUpdate(delta);
				}

				nSteps++;
				curTime = api->system->getTicks();
				targetTime = startTime + Uint32(((long long)nSteps * 1000) / fps);
			}
		} else {
			// Nope, release CPU
			api->system->delay(1);
		}

		// Variable step
		if (running) {
			onVariableUpdate(delta);
		}

		// Render screen
		if (running) {
			onRender(delta);
		}

		// Switch stage
		transitionStage();
	}

	Debug::trace("Game::runMainLoop end");
	std::cout << ConsoleColor(Console::GREEN) << "Main loop terminated." << ConsoleColor() << std::endl;
}
