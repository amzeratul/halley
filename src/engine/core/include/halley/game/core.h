#pragma once

#include <memory>
#include <halley/data_structures/vector.h>
#include <halley/time/halleytime.h>
#include <halley/time/stopwatch.h>
#include <halley/support/redirect_stream.h>
#include <halley/stage/stage.h>
#include <halley/game/main_loop.h>
#include <halley/plugin/plugin.h>
#include <halley/api/halley_api_internal.h>
#include "halley_statics.h"
#include <halley/data_structures/tree_map.h>
#include "halley/support/logger.h"
#include "halley/support/profiler.h"
#include "halley/os/os.h"

namespace Halley
{
	class RenderSnapshot;
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
		using Clock = std::chrono::steady_clock;
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
		
		void onTick(Time delta) override;
		bool isRunning() const override	{ return running; }
		bool transitionStage() override;
		const HalleyAPI& getAPI() const override { return *api; }
		HalleyStatics& getStatics() override;

		Resources& getResources();

		void onSuspended() override;
		void onReloaded() override;
		void onTerminatedInError(const std::string& error) override;

		bool hasVsync() override;
		void waitForVsync() override;
		double getTargetFPS() override;

		void registerDefaultPlugins();
		void registerPlugin(std::unique_ptr<Plugin> plugin) override;
		Vector<Plugin*> getPlugins(PluginType type) override;

		void log(LoggerLevel level, const std::string_view msg) override;

		void addProfilerCallback(IProfileCallback* callback) override;
		void removeProfilerCallback(IProfileCallback* callback) override;
		void addStartFrameCallback(IStartFrameCallback* callback) override;
		void removeStartFrameCallback(IStartFrameCallback* callback) override;

		Future<std::unique_ptr<RenderSnapshot>> requestRenderSnapshot() override;

		int getExitCode() const { return exitCode; }

		DevConClient* getDevConClient() const override;

	private:
		void deInit();

		void initResources();
		void setOutRedirect(bool appendToExisting);

		void tickFrame(Time time);
		void render();
		void waitForRenderEnd();
		void updateFrameData(bool multithreaded, Time time);

		void showComputerInfo() const;

		void processEvents(Time time);
		void clearPresses();
		void runStartFrame(Time time);
		void update(Time time, bool multithreaded);
		void preUpdate(Time time);
		std::pair<size_t, Time> getFixedUpdateCount(Time time);
		void fixedUpdate(Time time, bool multithreaded);
		void variableUpdate(Time time);
		void postUpdate(Time time);
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
		Time fixedUpdateTime = 0;

		std::unique_ptr<BaseFrameData> frameDataUpdate;
		std::unique_ptr<BaseFrameData> frameDataRender;

		bool initialized = false;
		bool running = true;
		bool hasError = false;
		bool hasConsole = false;
		int exitCode = 0;
		std::unique_ptr<RedirectStream> out;

		std::unique_ptr<DevConClient> devConClient;

		Vector<IProfileCallback*> profileCallbacks;
		Vector<Promise<std::unique_ptr<RenderSnapshot>>> pendingSnapshots;

		Vector<IStartFrameCallback*> startFrameCallbacks;
		
		TreeMap<PluginType, Vector<std::unique_ptr<Plugin>>> plugins;
		HalleyStatics statics;

		ComputerData computerData;
	};
}
