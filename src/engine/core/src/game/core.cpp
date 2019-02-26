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
#include <chrono>
#include <ctime>
#include "../dummy/dummy_plugins.h"
#include "halley/core/devcon/devcon_client.h"
#include "halley/net/connection/network_service.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

using namespace Halley;

Core::Core(std::unique_ptr<Game> g, Vector<std::string> _args)
{
	statics.setupGlobals();
	Logger::addSink(*this);

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
		OS::get().initializeConsole();
	}
	setOutRedirect(false);

	std::cout << ConsoleColour(Console::GREEN) << "Halley is initializing..." << ConsoleColour() << std::endl;

	// Debugging initialization
	Debug::setErrorHandling((environment->getDataPath() / "stack.dmp").string(), [=] (const std::string& error) { onTerminatedInError(error); });

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

	// Computer info
#ifndef _DEBUG
	showComputerInfo();
#endif

	// Create API
	registerDefaultPlugins();
	api = HalleyAPI::create(this, game->initPlugins(*this));
}

Core::~Core()
{
	deInit();
}

const HalleyStatics& Core::getStatics()
{
	return statics;
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
	statics.resume(api->system);
	if (api->system) {
		api->system->setThreadName("main");
	}
	
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
	if (!error.empty()) {
		std::cout << ConsoleColour(Console::RED) << "\n\nUnhandled exception: " << ConsoleColour(Console::DARK_RED) << error << ConsoleColour() << std::endl;
	} else {
		std::cout << ConsoleColour(Console::RED) << "\n\nUnknown unhandled exception." << ConsoleColour() << std::endl;
	}

	OS::get().displayError(error);

	std::cout << ConsoleColour(Console::RED) << "Last traces:\n" << ConsoleColour(Console::DARK_RED);
	Debug::printLastTraces();
	std::cout << "\nEnd of traces." << ConsoleColour() << std::endl;
	hasError = true;
}

int Core::getTargetFPS()
{
	return game->getTargetFPS();
}

void Core::init()
{
	// Initialize API
	api->init();
	api->systemInternal->setEnvironment(environment.get());
	statics.resume(api->system);
	if (api->system) {
		api->system->setThreadName("main");
	}

	// Resources
	initResources();

	// Create devcon connection
	String devConAddress = game->getDevConAddress();
	if (!devConAddress.isEmpty()) {
		devConClient = std::make_unique<DevConClient>(*api, api->network->createService(NetworkProtocol::TCP), devConAddress, game->getDevConPort());
	}

	// Start game
	setStage(game->startGame(&*api));
	
	// Get video resources
	if (api->video) {
		painter = api->videoInternal->makePainter(api->core->getResources());
	}
}

void Core::deInit()
{
	std::cout << "Game shutting down." << std::endl;

	// Ensure stage is cleaned up
	running = false;
	transitionStage();

	// Deinit game
	game->endGame();
	game.reset();

	// Terminate devcon
	if (devConClient) {
		devConClient.reset();
	}

	// Deinit painter
	painter.reset();

	// Stop audio playback before releasing resources
	if (api->audio) {
		api->audio->stopPlayback();
	}

	// Deinit resources
	resources.reset();

	// Deinit API (note that this has to happen after resources, otherwise resources which rely on an API to de-init, such as textures, will crash)
	api.reset();
	
	// Stop thread pool and other statics
	statics.suspend();

	// Deinit console redirector
	std::cout << "Goodbye!" << std::endl;
	std::cout.flush();
	Logger::removeSink(*this);
	out.reset();

#if defined(_WIN32) && !defined(WINDOWS_STORE)
	if (hasError && hasConsole) {
		system("pause");
	}
#endif
}

void Core::initResources()
{
	auto locator = std::make_unique<ResourceLocator>(*api->system);
	auto gamePath = environment->getProgramPath();
	game->initResourceLocator(gamePath, api->system->getAssetsPath(gamePath.string()), api->system->getUnpackedAssetsPath(gamePath.string()), *locator);
	resources = std::make_unique<Resources>(std::move(locator), &*api);
	StandardResources::initialize(*resources);
	api->audioInternal->setResources(*resources);
}

void Core::setOutRedirect(bool appendToExisting)
{
#if defined(_WIN32) || defined(__APPLE__) || defined(linux)
	String path = (Path(environment->getDataPath()) / "log.txt").getString();
#if defined(_WIN32) && !defined(__MINGW32__)
	auto outStream = std::make_shared<std::ofstream>(path.getUTF16().c_str(), appendToExisting ? std::ofstream::app : std::ofstream::trunc);
#else
	auto outStream = std::make_shared<std::ofstream>(path.c_str(), appendToExisting ? std::ofstream::app : std::ofstream::trunc);
#endif
	if (!outStream->is_open()) {
		outStream.reset();
	}
	out = std::make_unique<RedirectStreamToStream>(std::cout, outStream, false);
#endif
}

void Core::pumpEvents(Time time)
{
	auto video = dynamic_cast<VideoAPIInternal*>(&*api->video);
	auto input = dynamic_cast<InputAPIInternal*>(&*api->input);
	input->beginEvents(time);
	if (!api->system->generateEvents(video, input)) {
		quit(0); // System close event
	}

	if (devConClient) {
		devConClient->update();
	}
}

void Core::pumpAudio()
{
	if (api->audio) {
		HALLEY_DEBUG_TRACE();
		api->audioInternal->pump();
		HALLEY_DEBUG_TRACE();
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
	auto& engineTimer = engineTimers[int(TimeLine::FixedUpdate)];
	auto& gameTimer = gameTimers[int(TimeLine::FixedUpdate)];
	engineTimer.beginSample();

	gameTimer.beginSample();
	if (running && currentStage) {
		try {
			currentStage->onFixedUpdate(time);
		} catch (Exception& e) {
			game->onUncaughtException(e, TimeLine::FixedUpdate);
		}
	}
	gameTimer.endSample();
	pumpAudio();

	engineTimer.endSample();
	HALLEY_DEBUG_TRACE();
}

void Core::doVariableUpdate(Time time)
{
	HALLEY_DEBUG_TRACE();
	auto& engineTimer = engineTimers[int(TimeLine::VariableUpdate)];
	auto& gameTimer = gameTimers[int(TimeLine::VariableUpdate)];
	engineTimer.beginSample();

	pumpEvents(time);
	gameTimer.beginSample();
	if (running && currentStage) {
		try {
			currentStage->onVariableUpdate(time);
		} catch (Exception& e) {
			game->onUncaughtException(e, TimeLine::VariableUpdate);
		}
	}
	gameTimer.endSample();
	pumpAudio();

	if (api->platform) {
		api->platformInternal->update();
	}
	if (api->system) {
		api->systemInternal->update(time);
	}

	engineTimer.endSample();
	HALLEY_DEBUG_TRACE();
}

void Core::doRender(Time)
{
	HALLEY_DEBUG_TRACE();
	auto& engineTimer = engineTimers[int(TimeLine::Render)];
	auto& gameTimer = gameTimers[int(TimeLine::Render)];
	bool gameSampled = false;
	engineTimer.beginSample();

	if (api->video) {
		api->video->startRender();
		painter->startRender();

		if (currentStage) {
			auto windowSize = api->video->getWindow().getDefinition().getSize();
			if (windowSize != prevWindowSize) {
				screenTarget.reset();
				screenTarget = api->video->createScreenRenderTarget();
				camera = std::make_unique<Camera>(Vector2f(windowSize) * 0.5f);
				prevWindowSize = windowSize;
			}
			RenderContext context(*painter, *camera, *screenTarget);

			gameTimer.beginSample();

			try {
				currentStage->onRender(context);
			} catch (Exception& e) {
				game->onUncaughtException(e, TimeLine::Render);
			}

			gameTimer.endSample();
			gameSampled = true;
		}

		painter->endRender();

		vsyncTimer.beginSample();
		api->video->finishRender();
		vsyncTimer.endSample();
	}

	if (!gameSampled) {
		gameTimer.beginSample();
		gameTimer.endSample();
	}

	engineTimer.endSample();
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
	std::cout << "Game terminating via CoreAPI::quit(" << code << ")." << std::endl;
	running = false;
}

Resources& Core::getResources()
{
	Expects(resources);
	return *resources;
}

const Environment& Core::getEnvironment()
{
	Expects(environment);
	return *environment;
}

int64_t Core::getTime(CoreAPITimer timerType, TimeLine tl, StopwatchAveraging::Mode mode) const
{
	switch (timerType) {
	case CoreAPITimer::Engine:
		return engineTimers[int(tl)].elapsedNanoSeconds(mode);
	case CoreAPITimer::Game:
		return gameTimers[int(tl)].elapsedNanoSeconds(mode);
	case CoreAPITimer::Vsync:
		return vsyncTimer.elapsedNanoSeconds(mode);
	default:
		return 0;
	}
}

void Core::initStage(Stage& stage)
{
	stage.api = &*api;
	stage.setGame(*game);
	stage.init();
}

Stage& Core::getCurrentStage()
{
	return *currentStage;
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
			currentStage.reset();
			HALLEY_DEBUG_TRACE();
		}

		// Update stage
		currentStage = std::move(nextStage);

		// Prepare next stage
		if (currentStage) {
			HALLEY_DEBUG_TRACE();
			initStage(*currentStage);
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

void Core::registerDefaultPlugins()
{
	registerPlugin(std::make_unique<DummySystemPlugin>());
	registerPlugin(std::make_unique<DummyVideoPlugin>());
	registerPlugin(std::make_unique<DummyAudioPlugin>());
	registerPlugin(std::make_unique<DummyInputPlugin>());
	registerPlugin(std::make_unique<DummyNetworkPlugin>());
	registerPlugin(std::make_unique<DummyPlatformPlugin>());
	registerPlugin(std::make_unique<DummyMoviePlugin>());
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
	std::sort(result.begin(), result.end(), [] (Plugin* a, Plugin* b) { return a->getPriority() > b->getPriority(); });
	return result;
}

void Core::log(LoggerLevel level, const String& msg)
{
	if (level == LoggerLevel::Dev && (!game || !game->isDevMode())) {
		return;
	}

	if (level == LoggerLevel::Error) {
		std::cout << ConsoleColour(Console::RED);
	} else if (level == LoggerLevel::Warning) {
		std::cout << ConsoleColour(Console::YELLOW);
	}
	std::cout << msg << ConsoleColour() << std::endl;
}
