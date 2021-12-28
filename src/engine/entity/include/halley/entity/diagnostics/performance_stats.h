#pragma once

#include "stats_view.h"
#include "halley/core/api/core_api.h"
#include "halley/support/profiler.h"

namespace Halley
{
	class System;

	class PerformanceStatsView : public StatsView, public CoreAPI::IProfileCallback
	{
	public:
		PerformanceStatsView(Resources& resources, const HalleyAPI& api);
		~PerformanceStatsView() override;

		void update() override;
		void paint(Painter& painter) override;

		void onProfileData(std::shared_ptr<ProfilerData> data) override;

	private:
		class FrameData {
		public:
			int variableTime = 0;
			int fixedTime = 0;
			int renderTime = 0;
		};
		
		TextRenderer headerText;
		TextRenderer graphFPS;

		int64_t totalFrameTime = 0;
		int64_t vsyncTime = 0;
		
		std::vector<FrameData> frameData;
		size_t lastFrameData = 0;

		const Sprite timelineBg;
		const Sprite whitebox;
		
		std::shared_ptr<ProfilerData> lastProfileData;

		void drawHeader(Painter& painter);
		void drawTimeline(Painter& painter, Rect4f rect);
		void drawTimeGraph(Painter& painter, Rect4f rect);
		void drawTimeGraphThread(Painter& painter, Rect4f rect, const ProfilerData::ThreadInfo& threadInfo);

		int64_t getTimeNs(TimeLine timeline, const ProfilerData& data);
	};
}
