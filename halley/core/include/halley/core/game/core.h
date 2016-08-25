#pragma once

#include <memory>
#include <halley/data_structures/vector.h>
#include <halley/time/halleytime.h>
#include <halley/time/stopwatch.h>
#include <halley/support/redirect_stream.h>
#include <halley/core/stage/stage.h>
#include <halley/runner/main_loop.h>
#include <halley/plugin/plugin.h>
#include <halley/core/api/halley_api_internal.h>
#include "halley_statics.h"
#include <halley/data_structures/tree_map.h>

namespace Halley
{
	class Game;
	class HalleyAPI;
	class Stage;
	class Painter;
	class Camera;
	class RenderTarget;
	class Environment;

	class Core final : public CoreAPIInternal, public IMainLoopable
	{
	public:
		Core(std::unique_ptr<Game> game, Vector<std::string> args);
		~Core();
		void init() override;

		void setStage(StageID stage) override;
		void setStage(std::unique_ptr<Stage> stage);
		void quit() override;
		Resources& getResources() override;
		long long getAverageTime(TimeLine tl) const override;
		long long getElapsedTime(TimeLine tl) const override;

		void onFixedUpdate(Time time) override;
		void onVariableUpdate(Time time) override;
		bool isRunning() const override	{ return running; }
		bool transitionStage() override;
		HalleyAPI& getAPI() const override { return *api; }

		void onSuspended() override;
		void onReloaded() override;

		void registerPlugin(std::unique_ptr<Plugin> plugin) override;
		Vector<Plugin*> getPlugins(PluginType type) override;

	private:
		void deInit();

		void initResources();
		void setOutRedirect(bool appendToExisting);

		void doFixedUpdate(Time time);
		void doVariableUpdate(Time time);
		void doRender(Time time);

		void showComputerInfo() const;

		void pumpEvents(Time time);

		std::array<StopwatchAveraging, int(TimeLine::NUMBER_OF_TIMELINES)> timers;

		Vector<String> args;

		std::unique_ptr<Environment> environment;
		std::unique_ptr<Game> game;
		std::unique_ptr<HalleyAPI> api;
		std::unique_ptr<Resources> resources;

		std::unique_ptr<Painter> painter;
		std::unique_ptr<Camera> camera;
		std::unique_ptr<RenderTarget> screenTarget;

		std::unique_ptr<Stage> currentStage;
		std::unique_ptr<Stage> nextStage;
		bool pendingStageTransition = false;

		bool running = true;
		std::unique_ptr<RedirectStream> out;

		TreeMap<PluginType, Vector<std::unique_ptr<Plugin>>> plugins;
		HalleyStatics statics;
	};
}
