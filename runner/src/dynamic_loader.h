#pragma once
#include <halley/text/halleystring.h>
#include <memory>
#include <halley/runner/game_loader.h>
#include "dynamic_library.h"

namespace Halley
{
	class IHalleyEntryPoint;
	class Game;

	class DynamicGameLoader : public GameLoader
	{
	public:
		DynamicGameLoader(String dllName);
		~DynamicGameLoader();

		std::unique_ptr<Game> createGame() override;
		bool needsToReload() const override;
		void reload() override;
		void setCore(Core& core) override;

	private:
		DynamicLibrary lib;
		IHalleyEntryPoint* entry = nullptr;
		Core* core = nullptr;

		void load();
		void unload();
		void hotPatch();
		void setStatics();
	};
}
