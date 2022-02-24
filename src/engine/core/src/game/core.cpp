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
#include "entry/entry_point.h"
#include "halley/core/devcon/devcon_client.h"
#include "halley/net/connection/network_service.h"
#include "halley/support/profiler.h"
#include "halley/utils/algorithm.h"
#include "halley/utils/halley_iostream.h"

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
		const auto info = game->getConsoleInfo();
		OS::get().createLogConsole(info.name, info.monitor, info.monitorAlign);
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

HalleyStatics& Core::getStatics()
{
	return statics;
}

void Core::onSuspended()
{
	HALLEY_DEBUG_TRACE();
	if (api->videoInternal) {
		api->videoInternal->onSuspend();
	}
	if (api->audioInternal) {
		api->audioInternal->onSuspend();
	}
	if (api->audioOutputInternal) {
		api->audioOutputInternal->onSuspend();
	}
	if (api->inputInternal) {
		api->inputInternal->onSuspend();
	}
	if (api->systemInternal) {
		api->systemInternal->onSuspend();
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

	if (api->systemInternal) {
		api->systemInternal->onResume();
	}
	if (api->inputInternal) {
		api->inputInternal->onResume();
	}
	if (api->audioOutputInternal) {
		api->audioOutputInternal->onResume();
	}
	if (api->audioInternal) {
		api->audioInternal->onResume();
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

	OS::get().displayError((game ? game->getName() : String("Halley")) + " has aborted with an unhandled exception: \n\n" + error);

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
	Expects(!initialized);
	
	// Initialize API
	api->init();
	api->systemInternal->setEnvironment(environment.get());
	statics.resume(api->system);
	if (api->system) {
		api->system->setThreadName("main");
	}

	// Resources
	initResources();

	// Give game api and resources
	game->api = api.get();
	game->resources = resources.get();

	// Create devcon connection
	String devConAddress = game->getDevConAddress();
	if (!devConAddress.isEmpty()) {
		devConClient = std::make_unique<DevConClient>(*api, *resources, api->network->createService(NetworkProtocol::TCP), std::move(devConAddress), game->getDevConPort());
	}

	// Start game
	setStage(game->startGame());
	
	// Get video resources
	if (api->video) {
		painter = api->videoInternal->makePainter(*resources);
	}

	initialized = true;
}

void Core::deInit()
{
	std::cout << "Game shutting down." << std::endl;
	Expects(initialized);

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

	
	// Stop thread pool and other statics
	statics.suspend();
	
	// Deinit resources
	resources.reset();

	// Deinit API (note that this has to happen after resources, otherwise resources which rely on an API to de-init, such as textures, will crash)
	api->deInit();
	api.reset();
	initialized = false;

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
	auto options = game->initResourceLocator(gamePath, api->system->getAssetsPath(gamePath.string()), api->system->getUnpackedAssetsPath(gamePath.string()), *locator);
	resources = std::make_unique<Resources>(std::move(locator), *api, options);
	StandardResources::initialize(*resources);
	api->audioInternal->setResources(*resources);
}

void Core::setOutRedirect(bool appendToExisting)
{
#if defined(_WIN32) || defined(__APPLE__) || defined(linux)
	String path = (Path(environment->getDataPath()) / "log.txt").getString();
#ifdef _WIN32
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

void Core::processEvents(Time time)
{
	ProfilerEvent event(ProfilerEventType::CorePumpEvents);

	auto* video = dynamic_cast<VideoAPIInternal*>(&*api->video);
	auto* input = dynamic_cast<InputAPIInternal*>(&*api->input);
	input->beginEvents(time);

	if (api->system) {
		api->systemInternal->onTickMainLoop();

		if (!api->system->generateEvents(video, input)) {
			// System close event
			if (!currentStage || currentStage->onQuitRequested()) {
				quit(0);
			}
		}
	}
}

void Core::runStartFrame()
{
	if (currentStage) {
		currentStage->onStartFrame();
	}
}

void Core::runPreVariableUpdate(Time time)
{
	if (devConClient) {
		ProfilerEvent event(ProfilerEventType::CoreDevConClient);
		devConClient->update(time);
	}
}

void Core::runPostVariableUpdate(Time time)
{
	pumpAudio();
	updatePlatform();
	updateSystem(time);
}

void Core::pumpAudio()
{
	if (api->audio) {
		ProfilerEvent event(ProfilerEventType::CorePumpAudio);
		api->audioInternal->pump();
	}
}

void Core::updateSystem(Time time)
{
	if (api->system) {
		ProfilerEvent event(ProfilerEventType::CoreUpdateSystem);
		api->systemInternal->update(time);
	}
}

void Core::updatePlatform()
{
	if (api->platform) {
		ProfilerEvent event(ProfilerEventType::CoreUpdatePlatform);
		api->platformInternal->update();
	}
}

void Core::onFixedUpdate(Time time)
{
	if (isRunning()) {
		// TODO: move this to onTick
		//doFixedUpdate(time);
	}
}

void Core::onTick(Time time)
{
	auto& capture = ProfilerCapture::get();
	const bool record = !profileCallbacks.empty();
	capture.startFrame(record);
	
	processEvents(time);

	tickFrame(time);

	capture.endFrame();
	if (record && capture.getFrameTime() >= getProfileCaptureThreshold()) {
		onProfileData(std::make_shared<ProfilerData>(capture.getCapture()));
	}
}

void Core::tickFrame(Time time)
{
	if (!isRunning()) {
		return;
	}

	const bool multithreaded = currentStage && currentStage->hasMultithreadedRendering();

	runStartFrame();
	
	if (multithreaded) {
		auto updateTask = Concurrent::execute([&] () {
			runPreVariableUpdate(time);
			runVariableUpdate(time);
			runPostVariableUpdate(time);
		});
		if (curStageFrames > 0) {
			render();
			waitForRenderEnd();
		}
		updateTask.wait();
	} else {
		runPreVariableUpdate(time);
		runVariableUpdate(time);
		runPostVariableUpdate(time);
		if (isRunning()) { // Check again, it might have changed
			render();
			waitForRenderEnd();
		}
	}

	curStageFrames++;
}

void Core::doFixedUpdate(Time time)
{
	if (running && currentStage) {
		ProfilerEvent event(ProfilerEventType::CoreFixedUpdate);
		try {
			currentStage->onFixedUpdate(time);
		} catch (Exception& e) {
			game->onUncaughtException(e, TimeLine::FixedUpdate);
		}
	}
}

void Core::runVariableUpdate(Time time)
{		
	if (running && currentStage) {
		ProfilerEvent event(ProfilerEventType::CoreVariableUpdate);
		try {
			currentStage->onVariableUpdate(time);
		} catch (Exception& e) {
			game->onUncaughtException(e, TimeLine::VariableUpdate);
		}
	}
}

void Core::render()
{
	if (api->video) {
		{
			ProfilerEvent event(ProfilerEventType::CoreStartRender);
			api->video->startRender();
		}

		{
			ProfilerEvent event(ProfilerEventType::CoreRender);

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

				try {
					currentStage->onRender(context);
				} catch (Exception& e) {
					game->onUncaughtException(e, TimeLine::Render);
				}
			}

			painter->endRender();
		}
	}
}

void Core::waitForRenderEnd()
{
	if (api->video) {
		ProfilerEvent event(ProfilerEventType::CoreVSync);
		api->video->finishRender();
	}
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
	if (running) {
		exitCode = code;
		std::cout << "Game terminating via CoreAPI::quit(" << code << ")." << std::endl;
		running = false;
	}
}

Resources& Core::getResources()
{
	Expects(resources != nullptr);
	return *resources;
}

const Environment& Core::getEnvironment()
{
	Expects(environment != nullptr);
	return *environment;
}

bool Core::isDevMode()
{
	return game->isDevMode();
}

void Core::initStage(Stage& stage)
{
	stage.setGame(*game);
	stage.doInit(api.get(), *resources);
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
		curStageFrames = 0;

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
	} else if (level == LoggerLevel::Dev) {
		std::cout << ConsoleColour(Console::CYAN);
	}

	/*
	// Print timestamp
	const auto now = std::chrono::system_clock::now();
	const auto nowTimeT = std::chrono::system_clock::to_time_t(now);
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	tm buffer;
	localtime_s(&buffer, &nowTimeT);
	std::cout << std::put_time(&buffer, "%T")  << '.' << std::setfill('0') << std::setw(3) << ms.count() << " ";
	*/

	std::cout << msg << ConsoleColour() << std::endl;
}

void IHalleyEntryPoint::initSharedStatics(const HalleyStatics& parent)
{
	static HalleyStatics statics(parent);
	statics.setupGlobals();
}

void Core::onProfileData(std::shared_ptr<ProfilerData> data)
{
	for (auto* c: profileCallbacks) {
		c->onProfileData(data);
	}
}

Time Core::getProfileCaptureThreshold() const
{
	Time t = std::numeric_limits<Time>::infinity();
	for (const auto* c: profileCallbacks) {
		t = std::min(t, c->getThreshold());
	}
	return t;
}

void Core::addProfilerCallback(IProfileCallback* callback)
{
	if (!std_ex::contains(profileCallbacks, callback)) {
		profileCallbacks.push_back(callback);
	}
}

void Core::removeProfilerCallback(IProfileCallback* callback)
{
	std_ex::erase_if(profileCallbacks, [&] (const auto& c) { return c == callback; });
}
