#pragma once
#include "game_reloader.h"
#include <halley/text/halleystring.h>
#include <memory>
#include "game.h"

namespace Halley
{
	class DynamicGameReloader : public GameReloader
	{
	public:
		DynamicGameReloader(String dllName);
		std::unique_ptr<Game> createGame();
		bool needsToReload() const override;
		void reload() override;

	private:
		String dllName;
	};
}
