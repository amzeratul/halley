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
#include <halley/support/debug.h>
#include <chrono>
#include <ctime>

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

	// Time
	auto now = std::chrono::system_clock::now();
	auto now_c = std::chrono::system_clock::to_time_t(now);
	std::cout << "It is " << std::put_time(std::localtime(&now_c), "%F %T") << std::endl;

	// Seed RNG
	time_t curTime = time(nullptr);
	clock_t curClock = clock();
	int seed = static_cast<int>(curTime) ^ static_cast<int>(curClock) ^ 0x3F29AB51;
	srand(seed);

	// Info
	std::cout << "Program dir: " << ConsoleColour(Console::DARK_GREY) << environment->getProgramPath() << ConsoleColour() << std::endl;
	std::cout << "Data dir: " << ConsoleColour(Console::DARK_GREY) << environment->getDataPath() << ConsoleColour() << std::endl;
}

Core::~Core()
{
	deInit();
}

void Core::onSuspended()
{
	HALLEY_DEBUG_TRACE();
	if (api->videoInternal) {
		api->videoInternal->onSuspend();
	}
	if (api->inputInternal) {
		api->inputInternal->onSuspend();
	}

	statics.suspend();

	std::cout.flush();
	out.reset();
	HALLEY_DEBUG_TRACE();
}

void Core::onReloaded()
{
	HALLEY_DEBUG_TRACE();
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
	HALLEY_DEBUG_TRACE();
}

void Core::onTerminatedInError(const std::string& error)
{
	if (error != "") {
		std::cout << ConsoleColour(Console::RED) << "\n\nUnhandled exception: " << ConsoleColour(Console::DARK_RED) << error << ConsoleColour() << std::endl;
	} else {
		std::cout << ConsoleColour(Console::RED) << "\n\nUnknown unhandled exception." << ConsoleColour() << std::endl;
	}
	std::cout << ConsoleColour(Console::RED) << "Last traces:\n" << ConsoleColour(Console::DARK_RED) << Debug::getLastTraces() << ConsoleColour() << std::endl;
	hasError = true;
}

void Core::init()
{
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
	auto locator = std::make_unique<ResourceLocator>(*api->system);
	game->initResourceLocator(environment->getProgramPath(), *locator);
	resources = std::make_unique<Resources>(std::move(locator), &*api);
	StandardResources::initialize(*resources);
}

void Core::setOutRedirect(bool appendToExisting)
{
	auto outStream = std::make_shared<std::ofstream>((Path(environment->getDataPath()) / "log.txt").getString(), appendToExisting ? std::ofstream::app : std::ofstream::trunc);
	out = std::make_unique<RedirectStreamToStream>(std::cout, outStream, false);
}

void Core::pumpEvents(Time time)
{
	auto video = dynamic_cast<VideoAPIInternal*>(&*api->video);
	auto input = dynamic_cast<InputAPIInternal*>(&*api->input);
	input->beginEvents(time);
	if (!api->system->generateEvents(video, input)) {
		quit(0); // System close event
	}
}

void Core::pumpAudio()
{
	if (api->audio) {
		api->audioInternal->pump();
	}
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
		auto windowSize = api->video->getWindow().getDefinition().getSize();
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
	HALLEY_DEBUG_TRACE();
	auto& t = timers[int(TimeLine::FixedUpdate)];
	t.beginSample();

	if (running) {
		if (currentStage) {
			currentStage->onFixedUpdate(time);
		}
	}
	pumpAudio();

	t.endSample();
	HALLEY_DEBUG_TRACE();
}

void Core::doVariableUpdate(Time time)
{
	HALLEY_DEBUG_TRACE();
	auto& t = timers[int(TimeLine::VariableUpdate)];
	t.beginSample();

	pumpEvents(time);
	if (running) {
		if (currentStage) {
			currentStage->onVariableUpdate(time);
		}
	}
	pumpAudio();

	t.endSample();
	HALLEY_DEBUG_TRACE();
}

void Core::doRender(Time)
{
	HALLEY_DEBUG_TRACE();
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
	HALLEY_DEBUG_TRACE();
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
	HALLEY_DEBUG_TRACE();
	setStage(game->makeStage(stage));
	HALLEY_DEBUG_TRACE();
}

void Core::setStage(std::unique_ptr<Stage> next)
{
	nextStage = std::move(next);
	pendingStageTransition = true;
}

void Core::quit(int code)
{
	exitCode = code;
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
			HALLEY_DEBUG_TRACE();
			currentStage->deInit();
			currentStage.reset();
			HALLEY_DEBUG_TRACE();
		}

		// Update stage
		currentStage = std::move(nextStage);

		// Prepare next stage
		if (currentStage) {
			HALLEY_DEBUG_TRACE();
			currentStage->api = &*api;
			currentStage->init();
			HALLEY_DEBUG_TRACE();
		} else {
			quit(0);
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

void Core::log(LoggerLevel level, const String& msg)
{
	std::cout << msg << std::endl;
}
