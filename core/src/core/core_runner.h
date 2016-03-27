#pragma once

#include <memory>
#include <vector>

namespace Halley
{
	class Game;
	class HalleyAPI;
	class Stage;

	class CoreRunner
	{
	public:
		CoreRunner();
		~CoreRunner();

		int run(std::unique_ptr<Game> game, std::vector<String> args);
		void setStage(std::unique_ptr<Stage> stage);

	private:
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
