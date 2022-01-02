#pragma once

#include <memory>
#include <halley/data_structures/vector.h>
#include <halley/time/halleytime.h>
#include <halley/time/stopwatch.h>
#include <halley/support/redirect_stream.h>
#include <halley/core/stage/stage.h>
#include <halley/core/game/main_loop.h>
#include <halley/plugin/plugin.h>
#include <halley/core/api/halley_api_internal.h>
#include "halley_statics.h"
#include <halley/data_structures/tree_map.h>
#include "halley/support/logger.h"
#include "halley/support/profiler.h"

namespace Halley
{
	class Game;
	class HalleyAPI;
	class Stage;
	class Painter;
	class Camera;
	class RenderTarget;
	class Environment;
	class DevConClient;

	class Core final : public CoreAPIInternal, public IMainLoopable, public ILoggerSink
	{
	public:
		Core(std::unique_ptr<Game> game, Vector<std::string> args);
		~Core();
		void init() override;

		void setStage(StageID stage) override;
		void setStage(std::unique_ptr<Stage> stage) override;
		void initStage(Stage& stage) override;
		Stage& getCurrentStage() override;
		void quit(int exitCode = 0) override;
		const Environment& getEnvironment() override;
		bool isDevMode() override;
		
		void onFixedUpdate(Time time) override;
		void onTick(Time time) override;
		bool isRunning() const override	{ return running; }
		bool transitionStage() override;
		const HalleyAPI& getAPI() const override { return *api; }
		HalleyStatics& getStatics() override;

		Resources& getResources();

		void onSuspended() override;
		void onReloaded() override;
		void onTerminatedInError(const std::string& error) override;
		int getTargetFPS() override;

		void registerDefaultPlugins();
		void registerPlugin(std::unique_ptr<Plugin> plugin) override;
		Vector<Plugin*> getPlugins(PluginType type) override;

		void log(LoggerLevel level, const String& msg) override;

		void addProfilerCallback(IProfileCallback* callback) override;
		void removeProfilerCallback(IProfileCallback* callback) override;

		int getExitCode() const { return exitCode; }

	private:
		void deInit();

		void initResources();
		void setOutRedirect(bool appendToExisting);

		void doFixedUpdate(Time time);
		void tickFrame(Time time);
		void render();
		void waitForRenderEnd();

		void showComputerInfo() const;

		void processEvents(Time time);
		void runStartFrame();
		void runPreVariableUpdate(Time time);
		void runVariableUpdate(Time time);
		void runPostVariableUpdate(Time time);
		void pumpAudio();
		void updateSystem(Time time);
		void updatePlatform();

		void onProfileData(std::shared_ptr<ProfilerData> data);
		Time getProfileCaptureThreshold() const;

		Vector<String> args;

		std::unique_ptr<Environment> environment;
		std::unique_ptr<Game> game;
		std::unique_ptr<HalleyAPI> api;
		std::unique_ptr<Resources> resources;

		std::unique_ptr<Painter> painter;
		std::unique_ptr<Camera> camera;
		std::unique_ptr<RenderTarget> screenTarget;
		Vector2i prevWindowSize = Vector2i(-1, -1);

		std::unique_ptr<Stage> currentStage;
		std::unique_ptr<Stage> nextStage;
		uint32_t curStageFrames = 0;
		bool pendingStageTransition = false;

		bool initialized = false;
		bool running = true;
		bool hasError = false;
		bool hasConsole = false;
		int exitCode = 0;
		std::unique_ptr<RedirectStream> out;

		std::unique_ptr<DevConClient> devConClient;

		std::vector<IProfileCallback*> profileCallbacks;
		
		TreeMap<PluginType, Vector<std::unique_ptr<Plugin>>> plugins;
		HalleyStatics statics;
	};
}
