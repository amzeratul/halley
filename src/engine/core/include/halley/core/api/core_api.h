#pragma once
#include "halley/core/stage/stage_id.h"
#include <halley/time/halleytime.h>
#include "halley/core/game/environment.h"
#include "halley/time/stopwatch.h"
#include <cstdint>

namespace Halley
{
	class Resources;
	class Stage;
	class HalleyStatics;

	enum class CoreAPITimer
	{
		Engine,
		Game,
		Vsync
	};

	class CoreAPI
	{
	public:
		virtual ~CoreAPI() {}
		virtual void quit(int exitCode = 0) = 0;
		virtual void setStage(StageID stage) = 0;
		virtual void setStage(std::unique_ptr<Stage> stage) = 0;
		virtual void initStage(Stage& stage) = 0;

		virtual const HalleyStatics& getStatics() = 0;
		
		virtual Resources& getResources() = 0;
		virtual const Environment& getEnvironment() = 0;

		virtual int64_t getTime(CoreAPITimer timer, TimeLine tl, StopwatchAveraging::Mode mode) const = 0;
	};
}
