#pragma once
#include <halley/text/halleystring.h>
#include <memory>
#include <halley/runner/game_loader.h>

namespace Halley
{
	class Game;

	class DynamicGameLoader : public GameLoader
	{
	public:
		DynamicGameLoader(String dllName);

		std::unique_ptr<Game> createGame() override;
		bool needsToReload() const override;
		void reload() override;

	private:
		String dllName;
	};
}
