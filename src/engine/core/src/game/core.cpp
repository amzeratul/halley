#include <iostream>
#include "halley/game/core.h"
#include "halley/game/game.h"
#include "halley/game/environment.h"
#include "halley/api/halley_api.h"
#include "halley/graphics/camera.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_target_screen.h"
#include "halley/graphics/window.h"
#include "halley/resources/resources.h"
#include "halley/resources/resource_locator.h"
#include "halley/resources/standard_resources.h"
#include <halley/os/os.h>
#include <halley/support/debug.h>
#include <halley/support/console.h>
#include <halley/concurrency/concurrent.h>
#include <fstream>
#include <chrono>
#include <ctime>
#include "../dummy/dummy_plugins.h"
#include "halley/entry/entry_point.h"
#include "halley/graphics/render_snapshot.h"
#include "halley/devcon/devcon_client.h"
#include "halley/input/input_joystick.h"
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
	if (!_args.empty()) {
		environment->parseProgramPath(_args[0]);
		args.resize(_args.size() - 1);
		std::copy(_args.begin() + 1, _args.end(), args.begin());
	}
	environment->setDataPath(game->getDataPath(args));
	environment->setArguments(_args);

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

	std::cout << ConsoleColour(Console::GREEN) << "Halley v" << getHalleyVersion().toString() << " is initializing..." << ConsoleColour() << std::endl;

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
#ifdef DEV_BUILD
	computerData = OS::get().getComputerData();
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
	statics.resume(api->system, game->getMaxThreads());
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

bool Core::hasVsync()
{
	return api && api->video && api->video->hasVsync();
}

void Core::waitForVsync()
{
	if (api && api->video) {
		api->video->waitForVsync();
	}
}

double Core::getTargetFPS()
{
	if (api && api->video && api->video->hasWindow()) {
		const auto& window = api->video->getWindow().getDefinition();
		if (!window.isFocusLost() && window.getWindowState() != WindowState::Minimized) {
			return game->getTargetFPS();
		}
	}

	return game->getTargetBackgroundFPS();
}

void Core::init()
{
	Expects(!initialized);
	initialized = true;
	
	// Initialize API
	api->init();
	api->systemInternal->setEnvironment(environment.get());
	statics.resume(api->system, game->getMaxThreads());
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
}

DevConClient* Core::getDevConClient() const
{
	return devConClient.get();
}

void Core::deInit()
{
	std::cout << "Game shutting down." << std::endl;
	Expects(initialized);
	initialized = false;

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
	api->inputInternal->setResources(*resources);
}

void Core::setOutRedirect(bool appendToExisting)
{
#if defined(_WIN32) || defined(__APPLE__) || defined(linux)
	String path = (Path(environment->getDataPath()) / game->getLogFileName()).getString();
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

void Core::clearPresses()
{
	auto& input = *api->input;
	const auto nJoy = input.getNumberOfJoysticks();
	const auto nKey = input.getNumberOfKeyboards();
	const auto nMice = input.getNumberOfMice();
	for (size_t i = 0; i < nJoy; ++i) {
		input.getJoystick(static_cast<int>(i))->clearPresses();
	}
	for (size_t i = 0; i < nKey; ++i) {
		input.getKeyboard(static_cast<int>(i))->clearPresses();
	}
	for (size_t i = 0; i < nMice; ++i) {
		input.getMouse(static_cast<int>(i))->clearPresses();
	}
}

void Core::runStartFrame(Time time)
{
	if (currentStage) {
		currentStage->onStartFrame(time, *frameDataUpdate);
	}
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

void Core::onTick(Time delta)
{
	auto& capture = ProfilerCapture::get();
	const bool record = !profileCallbacks.empty();
	capture.startFrame(record);
	
	tickFrame(delta);

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

	for (auto* c: startFrameCallbacks) {
		c->onStartFrame();
	}

	const bool multithreaded = currentStage && currentStage->hasMultithreadedRendering();

	startFrameData(multithreaded, time);
	runStartFrame(time);

	if (multithreaded || !game->shouldProcessEventsOnFixedUpdate()) {
		processEvents(time);
	}
	
	if (multithreaded) {
		auto updateTask = Concurrent::execute([&] () {
			BaseFrameData::setThreadFrameData(frameDataUpdate.get());
			update(time, multithreaded);
		});
		if (frameDataRender) {
			assert(curStageFrames > 0);
			BaseFrameData::setThreadFrameData(frameDataRender.get());
			render();
			waitForRenderEnd();
		}
		updateTask.wait();
	} else {
		BaseFrameData::setThreadFrameData(frameDataUpdate.get());
		update(time, multithreaded);
		if (isRunning()) { // Check again, it might have changed
			render();
			waitForRenderEnd();
		}
	}

	endFrameData(multithreaded, time);
	BaseFrameData::setThreadFrameData(nullptr);

	curStageFrames++;
}

void Core::update(Time time, bool multithreaded)
{
	// Run pre update, then ONE fixed update (if needed), then variable, then remaining fixed updates, with input cleared. This makes sure that input is consistent.
	preUpdate(time);

	auto [nFixed, fixedLen] = getFixedUpdateCount(time);
	if (nFixed > 0) {
		fixedUpdate(fixedLen, multithreaded);
	}

	variableUpdate(time);

	if (nFixed > 1) {
		clearPresses();
		for (size_t n = 1; n < nFixed; ++n) {
			fixedUpdate(fixedLen, multithreaded);
			if (n == 4) {
				// Don't let it run more than 5 fixed frames per variable frame
				fixedUpdateTime = 0;
				break;
			}
		}
	}

	postUpdate(time);
}

void Core::preUpdate(Time time)
{
	if (devConClient) {
		ProfilerEvent event(ProfilerEventType::CoreDevConClient);
		devConClient->update(time);
	}
}

void Core::postUpdate(Time time)
{
	pumpAudio();
	updatePlatform();
	updateSystem(time);
}

std::pair<size_t, Time> Core::getFixedUpdateCount(Time time)
{
	if (running && currentStage) {
		fixedUpdateTime += time;
		const Time fixedUpdatePeriod = 1.0 / game->getFixedUpdateFPS();
		const size_t nFixed = lroundl(std::floor((fixedUpdateTime + 0.0001) / fixedUpdatePeriod));
		return { nFixed, fixedUpdatePeriod };
	} else {
		return { 0, 0 };
	}
}

void Core::fixedUpdate(Time time, bool multithreaded)
{
	fixedUpdateTime = std::max(fixedUpdateTime - time, 0.0);
	
	if (!multithreaded && game->shouldProcessEventsOnFixedUpdate()) {
		processEvents(time);
	}

	ProfilerEvent event(ProfilerEventType::CoreFixedUpdate);
	try {
		currentStage->onFixedUpdate(time, *frameDataUpdate);
	} catch (Exception& e) {
		game->onUncaughtException(e, TimeLine::FixedUpdate);
	}
}

void Core::variableUpdate(Time time)
{		
	if (running && currentStage) {
		ProfilerEvent event(ProfilerEventType::CoreVariableUpdate);
		try {
			currentStage->onVariableUpdate(time, *frameDataUpdate);
		} catch (Exception& e) {
			game->onUncaughtException(e, TimeLine::VariableUpdate);
		}
	}
}

void Core::render()
{
	if (!api->video) {
		return;
	}

	if (!currentStage || !currentStage->canRender()) {
		return;
	}
		
	if (!painter) {
		painter = api->videoInternal->makePainter(*resources);
	}

	{
		ProfilerEvent event(ProfilerEventType::CoreStartRender);
		api->video->startRender();
	}

	std::unique_ptr<RenderSnapshot> snapshot;
	{
		ProfilerEvent event(ProfilerEventType::CoreRender);

		painter->startRender();

		if (!pendingSnapshots.empty()) {
			snapshot = std::make_unique<RenderSnapshot>();
			painter->startRecording(snapshot.get());
		} else if (game->canCollectVideoPerformance() && isDevMode()) {
			painter->startRecording(nullptr);
		}

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
				currentStage->onRender(context, *frameDataRender);
			}
			catch (Exception& e) {
				game->onUncaughtException(e, TimeLine::Render);
			}
		}

	}
	{
		painter->endRender();

		if (!pendingSnapshots.empty() && snapshot) {
			snapshot->finish();
			pendingSnapshots.front().setValue(std::move(snapshot));
			pendingSnapshots.erase(pendingSnapshots.begin());
		}
	}
}

void Core::waitForRenderEnd()
{
	if (api->video && painter) {
		bool hasVsync = true; // TODO
		ProfilerEvent event(ProfilerEventType::CoreVSync);
		api->video->finishRender();
		painter->onFinishRender();
	}
}

void Core::startFrameData(bool multithreaded, Time time)
{
	if (multithreaded) {
		std::swap(frameDataUpdate, frameDataRender);
	} else {
		frameDataRender = {};
	}
	if (!frameDataUpdate) {
		frameDataUpdate = game->makeFrameData();
		assert(!!frameDataUpdate);
	}
	frameDataUpdate->doStartFrame(multithreaded, multithreaded ? frameDataRender.get() : nullptr, time);
}

void Core::endFrameData(bool multithreaded, Time time)
{
	if (multithreaded) {
		if (frameDataRender) {
			frameDataRender->doEndFrame();
		}
	} else {
		frameDataUpdate->doEndFrame();
	}
}

void Core::showComputerInfo() const
{
	time_t rawtime;
	time(&rawtime);
	String curTime = asctime(localtime(&rawtime));
	curTime.trim(true);

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
        if (api && api->video) {
            api->video->flush();
        }

        // Get rid of current stage
		if (currentStage) {
			HALLEY_DEBUG_TRACE();
			currentStage.reset();
			HALLEY_DEBUG_TRACE();
		}

		// Update stage
		currentStage = std::move(nextStage);
		curStageFrames = 0;
		frameDataUpdate = {};
		frameDataRender = {};

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
	registerPlugin(std::make_unique<DummyAnalyticsPlugin>());
	registerPlugin(std::make_unique<DummyWebPlugin>());
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

void Core::log(LoggerLevel level, const std::string_view msg)
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

void Core::addStartFrameCallback(IStartFrameCallback* callback)
{
	if (!std_ex::contains(startFrameCallbacks, callback)) {
		startFrameCallbacks.push_back(callback);
	}
}

void Core::removeStartFrameCallback(IStartFrameCallback* callback)
{
	std_ex::erase_if(startFrameCallbacks, [&] (const auto& c) { return c == callback; });
}

Future<std::unique_ptr<RenderSnapshot>> Core::requestRenderSnapshot()
{
	auto& promise = pendingSnapshots.emplace_back();
	return promise.getFuture();
}

thread_local BaseFrameData* BaseFrameData::threadInstance = nullptr;
