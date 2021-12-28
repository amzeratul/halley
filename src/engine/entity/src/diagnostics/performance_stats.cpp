#include "diagnostics/performance_stats.h"

#include "system.h"
#include "world.h"
#include "halley/core/api/core_api.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/resources/resources.h"
#include "halley/support/logger.h"
#include "halley/support/profiler.h"
#include "halley/time/halleytime.h"
#include "halley/time/stopwatch.h"

using namespace Halley;

PerformanceStatsView::PerformanceStatsView(Resources& resources, const HalleyAPI& api)
	: StatsView(resources, api)
	, bg(Sprite().setImage(resources, "halley/perf_graph.png"))
	, whitebox(Sprite().setImage(resources, "whitebox.png"))
{
	api.core->addProfilerCallback(this);
	
	headerText = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f));
	graphFPS = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 15, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f))
		.setText("20\n\n30\n\n60").setAlignment(0.5f);

	constexpr size_t frameDataCapacity = 300;
	frameData.resize(frameDataCapacity);
	lastFrameData = frameDataCapacity - 1;
}

PerformanceStatsView::~PerformanceStatsView()
{
	api.core->removeProfilerCallback(this);
}

void PerformanceStatsView::update()
{
	StatsView::update();
}

void PerformanceStatsView::paint(Painter& painter)
{
	painter.setLogging(false);

	drawHeader(painter);
	drawGraph(painter, Vector2f(20, 80));
	
	painter.flush();
	painter.setLogging(true);
}


void PerformanceStatsView::onProfileData(std::shared_ptr<ProfilerData> data)
{
	vsyncTime = data->getElapsedTime(ProfilerEventType::CoreVSync).count();
	totalFrameTime = data->getTotalElapsedTime().count() - vsyncTime;

	auto getTime = [&](TimeLine timeline) -> int
	{
		const auto ns = getTimeNs(timeline, *data);
		return static_cast<int>((ns + 500) / 1000);
	};

	lastFrameData = (lastFrameData + 1) % frameData.size();
	auto& curFrameData = frameData[lastFrameData];
	curFrameData.fixedTime = getTime(TimeLine::FixedUpdate);
	curFrameData.variableTime = getTime(TimeLine::VariableUpdate);
	curFrameData.renderTime = getTime(TimeLine::Render);
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

int64_t PerformanceStatsView::getTimeNs(TimeLine timeline, const ProfilerData& data)
{
	// Total time
	const auto totalTime = data.frameEndTime - data.frameStartTime;

	// Render time
	const auto renderTime = data.getElapsedTime(ProfilerEventType::CoreRender);
	const auto vsyncTime = data.getElapsedTime(ProfilerEventType::CoreVSync);
	
	if (timeline == TimeLine::VariableUpdate) {
		return (totalTime - renderTime - vsyncTime).count();
	} else if (timeline == TimeLine::Render) {
		return renderTime.count();
	} else {
		return 0;
	}
}
