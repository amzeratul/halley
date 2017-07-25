#pragma once
#include "halley/core/stage/stage_id.h"
#include <halley/time/halleytime.h>
#include "halley/core/game/environment.h"

namespace Halley
{
	class Resources;
	class Stage;
	class HalleyStatics;

	class CoreAPI
	{
	public:
		virtual ~CoreAPI() {}
		virtual void quit(int exitCode = 0) = 0;
		virtual void setStage(StageID stage) = 0;
		virtual void setStage(std::unique_ptr<Stage> stage) = 0;

		virtual const HalleyStatics& getStatics() = 0;
		
		virtual Resources& getResources() = 0;
		virtual const Environment& getEnvironment() = 0;

		virtual long long getAverageTime(TimeLine tl) const = 0;
		virtual long long getElapsedTime(TimeLine tl) const = 0;
		virtual long long getVsyncTime() const = 0;
	};
}
