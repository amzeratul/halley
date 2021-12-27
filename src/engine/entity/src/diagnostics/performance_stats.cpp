#include "diagnostics/performance_stats.h"

#include "system.h"
#include "world.h"
#include "halley/core/api/core_api.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
#include "halley/time/halleytime.h"
#include "halley/time/stopwatch.h"

using namespace Halley;

PerformanceStatsView::PerformanceStatsView(Resources& resources, const HalleyAPI& api)
	: StatsView(resources, api)
	, bg(Sprite().setImage(resources, "halley/perf_graph.png"))
	, whitebox(Sprite().setImage(resources, "whitebox.png"))
{
	headerText = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f));
	timelineData[0].setText(headerText);
	timelineData[1].setText(headerText);
	timelineData[2].setText(headerText);
	graphFPS = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 15, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f))
		.setText("20\n\n30\n\n60").setAlignment(0.5f);

	constexpr size_t frameDataCapacity = 300;
	frameData.resize(frameDataCapacity);
	lastFrameData = frameDataCapacity - 1;
}

void PerformanceStatsView::update()
{
	StatsView::update();
	collectData();
}

void PerformanceStatsView::paint(Painter& painter)
{
	painter.setLogging(false);

	drawHeader(painter);
	drawGraph(painter, Vector2f(20, 80));
	drawTimeline(painter, "Fixed", TimeLine::FixedUpdate, Vector2f(25, 200));
	drawTimeline(painter, "Variable", TimeLine::VariableUpdate, Vector2f(25 + 410, 200));
	drawTimeline(painter, "Render", TimeLine::Render, Vector2f(25 + 410 * 2, 200));
	
	painter.flush();
	painter.setLogging(true);
}

void PerformanceStatsView::TimeLineData::setText(const TextRenderer& text)
{
	col0Text = text;
	col1Text = text;
	col2Text = text;
}

void PerformanceStatsView::collectData()
{
	totalFrameTime = 0;

	const TimeLine timelines[] = { TimeLine::FixedUpdate, TimeLine::VariableUpdate, TimeLine::Render };
	for (const auto timeline: timelines) {
		collectTimelineData(timeline);
	}
	//vsyncTime = api.core->getTime(CoreAPITimer::Vsync, TimeLine::Render, StopwatchRollingAveraging::Mode::Average);

	auto getTime = [&](TimeLine timeline) -> int
	{
		const auto ns = getTimeNs(timeline);
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
	/*
	auto& tl = timelineData[static_cast<int>(timeline)];
	auto& curTop = tl.topSystems;
	curTop.clear();

	auto& coreAPI = *api.core;
	const auto engineTime = coreAPI.getTime(CoreAPITimer::Engine, timeline, StopwatchRollingAveraging::Mode::Average);
	const auto gameTime = coreAPI.getTime(CoreAPITimer::Game, timeline, StopwatchRollingAveraging::Mode::Average);
	const auto engineTimeMax = coreAPI.getTime(CoreAPITimer::Engine, timeline, StopwatchRollingAveraging::Mode::Max);
	const auto gameTimeMax = coreAPI.getTime(CoreAPITimer::Game, timeline, StopwatchRollingAveraging::Mode::Max);
	
	tl.average = engineTime + gameTime;
	tl.max = engineTimeMax + gameTimeMax;
	totalFrameTime += tl.average;
	if (timeline == TimeLine::Render) {
		tl.average -= timer.averageElapsedNanoSeconds();
	}

	if (world) {
		for (const auto& system : world->getSystems(timeline)) {
			tryInsert(curTop, *system);
		}
	}
	*/
}

void PerformanceStatsView::tryInsert(std::vector<SystemData>& curTop, const System& system)
{
	const auto avg = system.getNanoSecondsTakenAvg();
	const auto max = system.getNanoSecondsTakenMax();
	const auto score = (avg + max) / 2;
	
	constexpr int systemsToTrack = 5;
	const int64_t minScore = curTop.size() < systemsToTrack ? -1 : curTop.back().score;
	if (curTop.size() < systemsToTrack) {
		curTop.emplace_back();
	}

	if (score > minScore) {
		auto& sys = curTop.back();
		sys.name = system.getName();
		sys.max = max;
		sys.average = avg;
		sys.score = score;

		std::sort(curTop.begin(), curTop.end(), [=](const SystemData& a, const SystemData& b) { return a.score > b.score; });
	}
}

void PerformanceStatsView::drawHeader(Painter& painter)
{
	const int curFPS = static_cast<int>(lround(1'000'000'000.0 / (totalFrameTime + vsyncTime)));
	const int maxFPS = static_cast<int>(lround(1'000'000'000.0 / totalFrameTime));

	
	String str = "Capped: " + formatTime(totalFrameTime + vsyncTime) + " ms [" + toString(curFPS) + " FPS] | Uncapped: " + formatTime(totalFrameTime) + " ms [" + toString(maxFPS) + " FPS].\n"
		+ toString(painter.getPrevDrawCalls()) + " draw calls, " + toString(painter.getPrevTriangles()) + " triangles, " + toString(painter.getPrevVertices()) + " vertices.";

	const auto audioSpec = api.audio->getAudioSpec();
	if (audioSpec) {
		const auto audioTime = api.audio->getLastTimeElapsed();
		const float totalTimePerBuffer = static_cast<float>(audioSpec->bufferSize) / static_cast<float>(audioSpec->sampleRate);
		const float audioTimeFloat = audioTime / 1'000'000'000.0f;
		const int percent = lround(audioTimeFloat / totalTimePerBuffer * 100.0f);
		str += "\nAudio time: " + formatTime(audioTime) + " ms (" + toString(percent) + "%)";
	}
	
	headerText
		.setText(str)
		.setPosition(Vector2f(20, 20))
		.draw(painter);
}

void PerformanceStatsView::drawTimeline(Painter& painter, const String& label, TimeLine timeline, Vector2f pos)
{
	String col0 = label;
	String col1 = "Avg";
	String col2 = "Max";

	auto addEntry = [&](const String str, int64_t avg, int64_t max)
	{
		col0 += "\n  " + str;
		col1 += "\n" + formatTime(avg);
		col2 += "\n" + formatTime(max);
	};

	auto& tl = timelineData[static_cast<int>(timeline)];
	addEntry("Total", tl.average, tl.max);
	
	int i = 1;
	for (const auto& system : tl.topSystems) {
		addEntry(toString(i) + ". " + system.name, system.average, system.max);
		++i;
	}
	
	tl.col0Text
		.setText(col0)
		.setPosition(pos)
		.draw(painter);
	tl.col1Text
		.setText(col1)
		.setPosition(pos + Vector2f(250, 0))
		.draw(painter);
	tl.col2Text
		.setText(col2)
		.setPosition(pos + Vector2f(320, 0))
		.draw(painter);
}

void PerformanceStatsView::drawGraph(Painter& painter, Vector2f pos)
{
	const Vector2f displaySize = Vector2f(1200, 100);
	const float maxFPS = 20.0f;
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

int64_t PerformanceStatsView::getTimeNs(TimeLine timeline)
{
	/*
	auto ns = api.core->getTime(CoreAPITimer::Engine, timeline, StopwatchRollingAveraging::Mode::Latest) + api.core->getTime(CoreAPITimer::Game, timeline, StopwatchRollingAveraging::Mode::Latest);
	if (timeline == TimeLine::Render) {
		ns -= timer.lastElapsedNanoSeconds();
	}
	return ns;
	*/
	return 1;
}
