#include <iostream>
#include <fstream>
#include "core_runner.h"
#include "game.h"
#include "environment.h"
#include "../api/halley_api.h"
#include "../prec.h"

#pragma warning(disable: 4996)

using namespace Halley;

CoreRunner::CoreRunner()
{
}

CoreRunner::~CoreRunner()
{
}

int CoreRunner::run(std::unique_ptr<Game> g, std::vector<String> args)
{
	game = std::move(g);
	
	bool initing = true;
	try {
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
	// Paths
	if (args.size() > 0) {
		Environment::parseProgramPath(args[0]);
	}
	Environment::setDataPath(game->getDataPath());

	// Redirect output
	auto outStream = std::make_shared<std::ofstream>(Environment::getDataPath() + "log.txt", std::ios::out);
	out = std::make_unique<RedirectStreamToStream>(std::cout, outStream, false);
	std::cout << "Data path is " << ConsoleColor(Console::DARK_GREY) << Environment::getDataPath() << ConsoleColor() << std::endl;

	// Computer info
#ifndef _DEBUG
	showComputerInfo();
#endif

	// API
	api = HalleyAPI::create(HalleyAPIFlags::Core | HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input);

	// Init game
	game->init(&*api);

	// Create frame
	setStage(game->makeStage(game->getInitialStage()));
}

void CoreRunner::deInit()
{
	// Deinit game
	game->deInit();

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

void CoreRunner::onFixedUpdate(Time time)
{
	running = api->core->processEvents(&*api->video, &*api->input);
	if (running) {
		if (currentStage) {
			currentStage->onFixedUpdate(time);
		}
	}
}

void CoreRunner::onVariableUpdate(Time time)
{
	running = api->core->processEvents(&*api->video, &*api->input);
	if (running) {
		if (currentStage) {
			currentStage->onUpdate(time);
		}
	}
}

void CoreRunner::onRender(Time time)
{
	if (api->video) {
		api->video->startRender();

		if (currentStage) {
			currentStage->onRender(time);
		}

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

void CoreRunner::setStage(std::unique_ptr<Stage> next)
{
	nextStage = std::move(next);
	pendingStageTransition = true;
}

void CoreRunner::transitionStage()
{
	if (pendingStageTransition) {
		if (currentStage) {
			currentStage->deInit();
			currentStage.reset();
		}

		currentStage = std::move(nextStage);

		if (currentStage) {
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
	Uint32 startTime = api->core->getTicks();
	Uint32 targetTime = startTime;
	Uint32 nSteps = 0;

	while (running) {
		Time delta = 1.0 / fps;

		if (delay > 0) {
			startTime += delay;
			targetTime += delay;
			delay = 0;
		}
		Uint32 curTime = api->core->getTicks();

		// Got anything to do?
		if (curTime >= targetTime) {
			// Step until we're up-to-date
			for (int i = 0; i < 10 && curTime >= targetTime; i++) {
				// Fixed step
				if (running) {
					onFixedUpdate(delta);
				}

				nSteps++;
				curTime = api->core->getTicks();
				targetTime = startTime + Uint32(((long long)nSteps * 1000) / fps);
			}
		} else {
			// Nope, release CPU
			api->core->delay(1);
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
