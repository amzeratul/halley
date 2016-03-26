#pragma once

#include <memory>
#include <vector>

namespace Halley
{
	class Game;
	class HalleyAPI;

	class CoreRunner
	{
	public:
		CoreRunner();
		~CoreRunner();
		int run(std::unique_ptr<Game> game, std::vector<String> args);

	private:
		void runMainLoop(bool capFrameRate, int fps);
		void init(std::vector<String> args);
		void deInit();

		void onFixedUpdate();
		void onVariableUpdate();
		void onRender();

		void handleException(std::exception& e);
		void showComputerInfo() const;

		std::unique_ptr<Game> game;
		std::unique_ptr<HalleyAPI> api;

		unsigned int delay;
		bool running = false;
		bool crashed = false;

		std::unique_ptr<RedirectStream> out;
	};
}
