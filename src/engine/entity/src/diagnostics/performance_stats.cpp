#include "diagnostics/performance_stats.h"

#include "system.h"
#include "world.h"
#include "halley/core/api/core_api.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/resources/resources.h"
#include "halley/time/halleytime.h"
#include "halley/time/stopwatch.h"

using namespace Halley;

PerformanceStatsView::PerformanceStatsView(Resources& resources, CoreAPI& coreAPI)
	: StatsView(resources, coreAPI)
	, whitebox(Sprite().setImage(resources, "whitebox.png"))
	, bg(Sprite().setImage(resources, "halley/perf_graph.png"))
{
	headerText = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f));
	topVariableText = headerText;
	topRenderText = headerText;
	graphFPS = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 15, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f))
		.setText("20\n\n30\n\n60").setAlignment(0.5f);

	constexpr size_t frameDataCapacity = 300;
	frameData.resize(frameDataCapacity);
	lastFrameData = frameDataCapacity - 1;
}

void PerformanceStatsView::update()
{
	collectData();
}

void PerformanceStatsView::paint(Painter& painter)
{
	painter.setLogging(false);

	drawHeader(painter);
	drawTimeline(painter, "Top Variable:", TimeLine::VariableUpdate, topVariableText, Vector2f(20, 300));
	drawTimeline(painter, "Top Render:", TimeLine::Render, topRenderText, Vector2f(650, 300));
	drawGraph(painter, Vector2f(20, 80));
	
	painter.flush();
	painter.setLogging(true);
}

void PerformanceStatsView::collectData()
{
	totalFrameTime = 0;

	const TimeLine timelines[] = { TimeLine::FixedUpdate, TimeLine::VariableUpdate, TimeLine::Render };
	for (const auto timeline: timelines) {
		collectTimelineData(timeline);
	}

	vsyncTime = coreAPI.getTime(CoreAPITimer::Vsync, TimeLine::Render, StopwatchAveraging::Mode::Average);

	auto getTime = [&](TimeLine timeline) -> int
	{
		auto ns = coreAPI.getTime(CoreAPITimer::Engine, timeline, StopwatchAveraging::Mode::Latest);
		if (timeline == TimeLine::Render) {
			ns -= coreAPI.getTime(CoreAPITimer::Vsync, timeline, StopwatchAveraging::Mode::Latest);
		}
		return static_cast<int>((ns + 500) / 1000);
	};
	
	lastFrameData = (lastFrameData + 1) % frameData.size();
	auto& curFrameData = frameData[lastFrameData];
	curFrameData.fixedTime = getTime(TimeLine::FixedUpdate);
	curFrameData.variableTime = getTime(TimeLine::VariableUpdate);
	curFrameData.renderTime = getTime(TimeLine::Render);
}

void PerformanceStatsView::collectTimelineData(TimeLine timeline)
{
	auto& curTop = topSystems[static_cast<int>(timeline)];
	curTop.clear();

	const int64_t total = coreAPI.getTime(CoreAPITimer::Engine, timeline, StopwatchAveraging::Mode::Average);
	totalFrameTime += total;

	if (world) {
		for (const auto& system : world->getSystems(timeline)) {
			tryInsert(curTop, *system);
		}
	}
}

void PerformanceStatsView::tryInsert(std::vector<SystemData>& curTop, const System& system)
{
	const auto avg = system.getNanoSecondsTakenAvg();
	constexpr int systemsToTrack = 5;
	const int64_t minAvg = curTop.size() < systemsToTrack ? -1 : curTop.back().average;
	if (curTop.size() < systemsToTrack) {
		curTop.emplace_back();
	}

	if (avg > minAvg) {
		auto& sys = curTop.back();
		sys.name = system.getName();
		sys.average = avg;

		std::sort(curTop.begin(), curTop.end(), [=](const SystemData& a, const SystemData& b) { return a.average > b.average; });
	}
}

void PerformanceStatsView::drawHeader(Painter& painter)
{
	const int curFPS = static_cast<int>(lround(1'000'000'000.0 / totalFrameTime));
	const int maxFPS = static_cast<int>(lround(1'000'000'000.0 / (totalFrameTime - vsyncTime)));

	String str = "Capped: " + formatTime(totalFrameTime) + " ms [" + toString(curFPS) + " FPS] | Uncapped: " + formatTime(totalFrameTime - vsyncTime) + " ms [" + toString(maxFPS) + " FPS].\n"
		+ toString(painter.getPrevDrawCalls()) + " draw calls, " + toString(painter.getPrevTriangles()) + " triangles, " + toString(painter.getPrevVertices()) + " vertices.\n";
	headerText
		.setText(str)
		.setPosition(Vector2f(20, 20))
		.draw(painter);
}

void PerformanceStatsView::drawTimeline(Painter& painter, const String& label, TimeLine timeline, TextRenderer& textRenderer, Vector2f pos)
{
	String str = label;
	int i = 1;
	for (const auto& system : topSystems[static_cast<int>(timeline)]) {
		str += "\n  " + toString(i) + ". " + system.name + std::string(32 - system.name.size(), ' ') + formatTime(system.average);
		++i;
	}
	textRenderer
		.setText(str)
		.setPosition(pos)
		.draw(painter);
}

void PerformanceStatsView::drawGraph(Painter& painter, Vector2f pos)
{
	const Vector2f displaySize = Vector2f(1200, 100);
	const float maxFPS = 30.0f;
	const float scale = maxFPS / 1'000'000.0f * displaySize.y;

	const Vector2f boxPos = pos + Vector2f(20, 0);
	bg
		.clone()
		.setPosition(boxPos - Vector2f(2, 2))
		.scaleTo(displaySize + Vector2f(4, 4))
		.draw(painter);

	auto variableSprite = whitebox
		.clone()
		.setPivot(Vector2f(0, 1))
		.setColour(Colour4f(0.5f, 0.4f, 1.0f));

	auto renderSprite = whitebox
		.clone()
		.setPivot(Vector2f(0, 1))
		.setColour(Colour4f(1.0f, 0.2f, 0.2f));

	const size_t n = frameData.size();
	const float oneOverN = 1.0f / static_cast<float>(n);
	const auto& xPos = [&](size_t i) -> float
	{
		return static_cast<float>(i) * displaySize.x * oneOverN;
	};

	for (size_t i = 0; i < n; ++i) {
		const size_t index = (i + lastFrameData + 1) % n;

		const float x = xPos(i);
		const float w = xPos(i + 1) - x;
		const Vector2f p = boxPos + Vector2f(x, displaySize.y);
		const Vector2f s1 = Vector2f(w, frameData[index].variableTime * scale);
		const Vector2f s2 = Vector2f(w, frameData[index].renderTime * scale);
		variableSprite
			.setPosition(p)
			.setSize(s1)
			.draw(painter);
		renderSprite
			.setPosition(p - Vector2f(0, s1.y))
			.setSize(s2)
			.draw(painter);
	}

	graphFPS.setPosition(pos + Vector2f(5.0f, 10.0f)).draw(painter);
	graphFPS.setPosition(pos + Vector2f(displaySize.x + 35.0f, 10.0f)).draw(painter);
}
