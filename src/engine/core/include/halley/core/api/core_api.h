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
	class ProfilerData;

	enum class CoreAPITimer
	{
		Engine,
		Game,
		Vsync
	};

	class CoreAPI
	{
	public:
		class IProfileCallback {
		public:
			virtual ~IProfileCallback() = default;
			virtual Time getThreshold() const { return 0.0; }
			virtual void onProfileData(std::shared_ptr<ProfilerData> data) = 0;
		};
		
		virtual ~CoreAPI() {}
		virtual void quit(int exitCode = 0) = 0;
		virtual void setStage(StageID stage) = 0;
		virtual void setStage(std::unique_ptr<Stage> stage) = 0;
		virtual void initStage(Stage& stage) = 0;
		virtual Stage& getCurrentStage() = 0;

		virtual HalleyStatics& getStatics() = 0;
		virtual const Environment& getEnvironment() = 0;

		virtual void addProfilerCallback(IProfileCallback* callback) = 0;
		virtual void removeProfilerCallback(IProfileCallback* callback) = 0;

		virtual bool isDevMode() = 0;
	};
}
