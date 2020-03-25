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
		int64_t getTime(CoreAPITimer timer, TimeLine tl, StopwatchAveraging::Mode mode) const override;

		void onFixedUpdate(Time time) override;
		void onVariableUpdate(Time time) override;
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

		int getExitCode() const { return exitCode; }

	private:
		void deInit();

		void initResources();
		void setOutRedirect(bool appendToExisting);

		void doFixedUpdate(Time time);
		void doVariableUpdate(Time time);
		void doRender(Time time);

		void showComputerInfo() const;

		void pumpEvents(Time time);
		void pumpAudio();

		std::array<StopwatchAveraging, int(TimeLine::NUMBER_OF_TIMELINES)> engineTimers;
		std::array<StopwatchAveraging, int(TimeLine::NUMBER_OF_TIMELINES)> gameTimers;
		StopwatchAveraging vsyncTimer;

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
		bool pendingStageTransition = false;

		bool initialized = false;
		bool running = true;
		bool hasError = false;
		bool hasConsole = false;
		int exitCode = 0;
		std::unique_ptr<RedirectStream> out;

		std::unique_ptr<DevConClient> devConClient;

		TreeMap<PluginType, Vector<std::unique_ptr<Plugin>>> plugins;
		HalleyStatics statics;
	};
}
