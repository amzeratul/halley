#pragma once

#include "stats_view.h"

namespace Halley
{
	class System;

	class PerformanceStatsView : public StatsView
	{
	public:
		PerformanceStatsView(Resources& resources, const HalleyAPI& api);

		void update() override;
		void paint(Painter& painter) override;

	private:
		class SystemData {
		public:
			String name;
			int64_t average = 0;
			int64_t max = 0;
			int64_t score = 0;
		};

		class TimeLineData {
		public:
			int64_t average = 0;
			int64_t max = 0;
			std::vector<SystemData> topSystems;
			TextRenderer col0Text;
			TextRenderer col1Text;
			TextRenderer col2Text;

			void setText(const TextRenderer& text);
		};

		class FrameData {
		public:
			int variableTime = 0;
			int fixedTime = 0;
			int renderTime = 0;
		};
		
		TextRenderer headerText;
		TextRenderer graphFPS;

		int64_t totalFrameTime;
		int64_t vsyncTime;
		
		std::array<TimeLineData, 3> timelineData;

		std::vector<FrameData> frameData;
		size_t lastFrameData = 0;

		const Sprite bg;
		const Sprite whitebox;

		void collectData();
		void collectTimelineData(TimeLine timeline);
		void tryInsert(std::vector<SystemData>& curTop, const System& system);

		void drawHeader(Painter& painter);
		void drawTimeline(Painter& painter, const String& label, TimeLine timeline, Vector2f pos);
		void drawGraph(Painter& painter, Vector2f pos);

		int64_t getTimeNs(TimeLine timeline);
	};
}
