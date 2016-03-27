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

	class CoreRunner : public CoreAPI
	{
	public:
		CoreRunner(std::unique_ptr<Game> game, std::vector<String> args);
		~CoreRunner();

		void setStage(StageID stage);
		void setStage(std::unique_ptr<Stage> stage);
		void quit();

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

		std::unique_ptr<Game> game;
		std::unique_ptr<HalleyAPI> api;

		std::unique_ptr<Stage> currentStage;
		std::unique_ptr<Stage> nextStage;
		bool pendingStageTransition = false;

		unsigned int delay;
		bool running = false;
		bool crashed = false;

		std::unique_ptr<RedirectStream> out;
	};
}
