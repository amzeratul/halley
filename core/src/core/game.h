#pragma once

#include "../stage/stage.h"

namespace Halley
{
	class Game
	{
	public:
		virtual ~Game() = default;

		virtual String getName() const = 0;
		virtual String getDataPath() const = 0;
		virtual bool isDevBuild() const = 0;

		virtual void init(HalleyAPI*) {}
		virtual void deInit() {}

		virtual std::unique_ptr<Stage> makeStage(StageID id) = 0;
		virtual StageID getInitialStage() const = 0;
	};
}
