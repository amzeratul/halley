#pragma once

#include "stats_view.h"

namespace Halley
{
	class System;

	class PerformanceStatsView : public StatsView
	{
	public:
		PerformanceStatsView(Resources& resources, CoreAPI& coreAPI);

		void update() override;
		void paint(Painter& painter) override;

	private:
		class SystemData {
		public:
			String name;
			int64_t average;
		};

		class FrameData {
		public:
			int variableTime = 0;
			int fixedTime = 0;
			int renderTime = 0;
		};
		
		TextRenderer headerText;
		TextRenderer topVariableText;
		TextRenderer topRenderText;

		int64_t totalFrameTime;
		int64_t vsyncTime;
		
		std::array<std::vector<SystemData>, 3> topSystems;

		std::vector<FrameData> frameData;
		size_t lastFrameData = 0;

		const Sprite whitebox;

		void collectData();
		void collectTimelineData(TimeLine timeline);
		void tryInsert(std::vector<SystemData>& curTop, const System& system);

		void drawHeader(Painter& painter);
		void drawTimeline(Painter& painter, const String& label, TimeLine timeline, TextRenderer& textRenderer, Vector2f pos);
		void drawGraph(Painter& painter, Vector2f pos);
	};
}
