#pragma once

#include <memory>
#include <vector>

namespace Halley
{
	class Game;

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

		void handleException(std::exception& e);

		std::unique_ptr<Game> game;
		bool running = false;
		bool crashed = false;

		std::unique_ptr<RedirectStream> out;
	};
}
