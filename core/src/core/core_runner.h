#pragma once

#include <memory>
#include <vector>
#include "../api/core_api.h"
#include "../stage/stage.h"

namespace Halley
{
	class Game;
	class HalleyAPI;
	class Stage;
	class Painter;
	class Camera;
	class RenderTarget;

	class CoreRunner : public CoreAPI
	{
	public:
		CoreRunner(std::unique_ptr<Game> game, std::vector<String> args);
		~CoreRunner();

		void setStage(StageID stage) override;
		void setStage(std::unique_ptr<Stage> stage);
		void quit() override;
		Resources& getResources() override;

	private:
		int run(std::unique_ptr<Game> game, std::vector<String> args);
		void runMainLoop(bool capFrameRate, int fps);
		void init(std::vector<String> args);
		void deInit();

		void onFixedUpdate(Time time);
		void onVariableUpdate(Time time);
		void onRender(Time time);

		void handleException(std::exception& e);
		void showComputerInfo() const;

		void transitionStage();
		void pumpEvents(Time time);

		std::unique_ptr<Game> game;
		std::unique_ptr<HalleyAPI> api;
		std::unique_ptr<Resources> resources;

		std::unique_ptr<Painter> painter;
		std::unique_ptr<Camera> camera;
		std::unique_ptr<RenderTarget> screenTarget;

		std::unique_ptr<Stage> currentStage;
		std::unique_ptr<Stage> nextStage;
		bool pendingStageTransition = false;

		unsigned int delay = 0;
		bool running = false;
		bool crashed = false;

		std::unique_ptr<RedirectStream> out;
	};
}
