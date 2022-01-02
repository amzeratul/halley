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
	, boxBg(Sprite().setImage(resources, "halley/box_2px_outline.png"))
	, whitebox(Sprite().setImage(resources, "whitebox.png"))
	, audioTime(60)
	, vsyncTime(60)
	, totalFrameTime(60)
{
	api.core->addProfilerCallback(this);
	
	headerText = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 16, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f));
	fpsLabel = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 15, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f))
		.setText("20\n\n30\n\n60").setAlignment(0.5f);
	graphLabel = TextRenderer(resources.get<Font>("Ubuntu Bold"), "", 15, Colour(1, 1, 1), 1.0f, Colour(0.1f, 0.1f, 0.1f)).setAlignment(0.5f);

	for (size_t i = 0; i < 8; ++i) {
		systemLabels.push_back(headerText.clone());
	}

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
	ProfilerEvent event(ProfilerEventType::StatsView);
	painter.setLogging(false);

	if (active) {
		whitebox.clone().setPosition(Vector2f(0, 0)).scaleTo(Vector2f(painter.getViewPort().getSize())).setColour(Colour4f(0, 0, 0, 0.5f)).draw(painter);
		
		drawHeader(painter, false);
		drawTimeline(painter, Rect4f(20, 80,	1240, 100));

		if (page == 0) {
			drawTimeGraph(painter, Rect4f(20, 200, 1240, 500));
		} else if (page == 1) {
			drawTopSystems(painter, Rect4f(20, 200, 1240, 500));
		}
	} else {
		drawHeader(painter, true);
	}
	
	painter.setLogging(true);
}

void PerformanceStatsView::onProfileData(std::shared_ptr<ProfilerData> data)
{
	vsyncTime.pushValue(data->getElapsedTime(ProfilerEventType::CoreVSync).count());
	totalFrameTime.pushValue(data->getTotalElapsedTime().count() - vsyncTime.getLatest());
	audioTime.pushValue(api.audio->getLastTimeElapsed());

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

	for (const auto& e: data->getEvents()) {
		if (e.type == ProfilerEventType::WorldSystemUpdate || e.type == ProfilerEventType::WorldSystemRender) {
			eventHistory[e.name].update(e.type, (e.endTime - e.startTime).count());
		}
	}

	if (capturing) {
		lastProfileData = std::move(data);
	}
}

int PerformanceStatsView::getPage() const
{
	return page;
}

void PerformanceStatsView::setPage(int page)
{
	this->page = page;
}

PerformanceStatsView::EventHistoryData::EventHistoryData()
	: average(120)
{
}

void PerformanceStatsView::EventHistoryData::update(ProfilerEventType type, int64_t value)
{
	this->type = type;
	const bool reset = average.pushValue(value);
	if (reset) {
		lastHighest = highest;
		lastLowest = lowest;
		highest = 0;
		lowest = std::numeric_limits<int64_t>::max();
	}
	highest = std::max(highest, value);
	lowest = std::min(lowest, value);
	highestEver = std::max(highestEver, value);
	lowestEver = std::min(lowestEver, value);
}

int64_t PerformanceStatsView::EventHistoryData::getAverage() const
{
	return average.getAverage();
}

int64_t PerformanceStatsView::EventHistoryData::getHighest() const
{
	return lastHighest;
}

int64_t PerformanceStatsView::EventHistoryData::getLowest() const
{
	return lastLowest;
}

int64_t PerformanceStatsView::EventHistoryData::getHighestEver() const
{
	return highestEver;
}

int64_t PerformanceStatsView::EventHistoryData::getLowestEver() const
{
	return lowestEver;
}

ProfilerEventType PerformanceStatsView::EventHistoryData::getType() const
{
	return type;
}

void PerformanceStatsView::drawHeader(Painter& painter, bool simple)
{
	const auto frameAvgTime = totalFrameTime.getAverage();
	const auto vsyncAvgTime = vsyncTime.getAverage();
	const auto audioAvgTime = audioTime.getAverage();
	const int curFPS = static_cast<int>(lround(1'000'000'000.0 / (frameAvgTime + vsyncAvgTime)));
	const int maxFPS = static_cast<int>(lround(1'000'000'000.0 / frameAvgTime));

	ColourStringBuilder strBuilder;
	strBuilder.append(toString(curFPS, 10, 3, ' '));
	strBuilder.append(" FPS / ");
	strBuilder.append(toString(maxFPS, 10, 4, ' '));
	strBuilder.append(" FPS / ");
	strBuilder.append(formatTime(frameAvgTime));
	strBuilder.append(" ms / ");
	strBuilder.append(toString(painter.getPrevDrawCalls()));
	strBuilder.append(" calls / ");
	strBuilder.append(toString(painter.getPrevTriangles()));
	strBuilder.append(" tris");

	if (!simple) {
		const auto audioSpec = api.audio->getAudioSpec();
		if (audioSpec) {
			const int64_t totalTimePerBuffer = int64_t(audioSpec->bufferSize) * 1'000'000'000 / int64_t(audioSpec->sampleRate);
			const auto percent = (audioAvgTime * 100.0f) / static_cast<float>(totalTimePerBuffer);
			strBuilder.append(" / ");
			strBuilder.append(formatTime(audioAvgTime));
			strBuilder.append(" ms audio (");
			strBuilder.append(toString(percent, 1));
			strBuilder.append("%)");
		}
	}

	if (simple) {
		headerText.setPosition(Vector2f(1260, 700)).setOffset(Vector2f(1, 1)).setOutline(2.0f);
	} else {
		headerText.setPosition(Vector2f(10, 10)).setOffset(Vector2f()).setOutline(1.0f);
	}

	auto [str, colours] = strBuilder.moveResults();
	
	headerText
		.setText(str)
		.setColourOverride(colours)
		.draw(painter);
}

void PerformanceStatsView::drawTimeline(Painter& painter, Rect4f rect)
{
	const Vector2f displaySize = rect.getSize() - Vector2f(40, 0);
	const auto pos = rect.getTopLeft();
	const float maxFPS = 20.0f;
	const float scale = maxFPS / 1'000'000.0f * displaySize.y;

	const Vector2f boxPos = pos + Vector2f(20, 0);
	boxBg
		.clone()
		.setPosition(boxPos - Vector2f(2, 2))
		.scaleTo(displaySize + Vector2f(4, 4))
		.draw(painter);

	// 30 FPS bar
	whitebox
		.clone()
		.setPosition(boxPos + Vector2f(0, displaySize.y / 3))
		.scaleTo(Vector2f(displaySize.x, 1))
		.setColour(Colour4f(0.75f))
		.draw(painter);

	// 60 FPS bar
	whitebox
		.clone()
		.setPosition(boxPos + Vector2f(0, 2 * displaySize.y / 3))
		.scaleTo(Vector2f(displaySize.x, 1))
		.setColour(Colour4f(0.75f))
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
		const Vector2f s1 = Vector2f(w, std::min(frameData[index].variableTime * scale, displaySize.y));
		const Vector2f s2 = Vector2f(w, std::min(frameData[index].renderTime * scale, displaySize.y - s1.y));
		variableSprite
			.setPosition(p)
			.setSize(s1)
			.draw(painter);
		renderSprite
			.setPosition(p - Vector2f(0, s1.y))
			.setSize(s2)
			.draw(painter);
	}

	fpsLabel.setPosition(pos + Vector2f(5.0f, 10.0f)).draw(painter);
	fpsLabel.setPosition(pos + Vector2f(displaySize.x + 35.0f, 10.0f)).draw(painter);
}

void PerformanceStatsView::drawTimeGraph(Painter& painter, Rect4f rect)
{
	if (!lastProfileData) {
		return;
	}

	const auto duration = std::chrono::duration<int64_t, std::nano>(16'999'999);
	const auto timeRange = Range<ProfilerData::TimePoint>(lastProfileData->getStartTime(), lastProfileData->getStartTime() + duration);
	const int64_t numMs = timeRange.getLength().count() / 1'000'000;

	boxBg
		.clone()
		.setPosition(rect.getTopLeft())
		.scaleTo(rect.getSize())
		.draw(painter);

	// Vertical dividers
	const float divPerNs = rect.getWidth() / static_cast<float>(timeRange.getLength().count());
	auto drawDivider = [&] (int64_t ns, Colour4f col, String label)
	{
		const float xPos = static_cast<float>(ns) * divPerNs;
		whitebox
			.clone()
			.setPosition(rect.getTopLeft() + Vector2f(xPos, 0))
			.scaleTo(Vector2f(1, rect.getHeight()))
			.setColour(col)
			.draw(painter);

		if (!label.isEmpty()) {
			graphLabel
				.setPosition(rect.getBottomLeft() + Vector2f(xPos, 0))
				.setColour(col)
				.setText(label)
				.draw(painter);
		}
	};

	// 1 ms dividers
	for (int64_t i = 1; i <= numMs; ++i) {
		drawDivider(i * 1'000'000, Colour4f(0.75f), toString(i) + " ms");
	}

	// Vsync divider
	drawDivider(16'666'666, Colour4f(1.0f, 0.75f, 0.0f), "vsync");

	drawTimeGraphThreads(painter, rect.shrink(2), timeRange);
}

void PerformanceStatsView::drawTimeGraphThreads(Painter& painter, Rect4f rect, Range<ProfilerData::TimePoint> timeRange)
{
	const auto& threads = lastProfileData->getThreads();

	int totalThreadDepth = 0;
	for (const auto& threadInfo: threads) {
		totalThreadDepth += threadInfo.maxDepth + 1;
	}

	const float threadSpacing = 20.0f;
	const float threadHeight = std::min(50.0f, std::floor((rect.getHeight() - (threads.size() - 1) * threadSpacing) / static_cast<float>(totalThreadDepth)));
	float heightSoFar = 0;
	for (const auto& threadInfo: threads) {
		const int depth = threadInfo.maxDepth + 1;
		drawTimeGraphThread(painter, Rect4f(rect.getLeft(), rect.getTop() + heightSoFar, rect.getWidth(), threadHeight * depth), threadInfo, timeRange);
		heightSoFar += static_cast<float>(depth) * threadHeight + threadSpacing;
	}
}

void PerformanceStatsView::drawTimeGraphThread(Painter& painter, Rect4f rect, const ProfilerData::ThreadInfo& threadInfo, Range<ProfilerData::TimePoint> timeRange)
{
	auto box = whitebox.clone();
	const auto frameStartTime = timeRange.start;
	const auto frameEndTime = timeRange.end;
	const auto frameLength = frameEndTime - frameStartTime;

	const auto origin = rect.getTopLeft();
	const auto lineHeight = std::floor(rect.getHeight() / static_cast<float>(threadInfo.maxDepth + 1));

	for (const auto& e: lastProfileData->getEvents()) {
		if (e.threadId == threadInfo.id) {
			const float relativeStart = static_cast<float>((e.startTime - frameStartTime).count()) / static_cast<float>(frameLength.count());
			const float relativeEnd = static_cast<float>((e.endTime - frameStartTime).count()) / static_cast<float>(frameLength.count());

			const float startPos = std::floor(relativeStart * rect.getWidth());
			const float endPos = std::floor(relativeEnd * rect.getWidth());
			const auto eventRect = origin + Rect4f(startPos, e.depth * lineHeight, std::max(endPos - startPos, 1.0f), lineHeight);

			const auto col = getEventColour(e.type);
			if (eventRect.getSize().x > 0.95f) {
				box
					.setColour(col.multiplyAlpha(0.5f))
					.setPosition(eventRect.getTopLeft() + Vector2f(1, 0))
					.scaleTo(eventRect.getSize() - Vector2f(1, 0))
					.draw(painter);
			}
			box
				.setColour(col)
				.scaleTo(Vector2f(1.0f, eventRect.getHeight()))
				.draw(painter);
		}
	}
}

Colour4f PerformanceStatsView::getEventColour(ProfilerEventType type) const
{
	switch (type) {
	case ProfilerEventType::CoreVSync:
		return Colour4f(0.8f, 0.8f, 0.1f);
	case ProfilerEventType::CoreUpdate:
	case ProfilerEventType::CoreFixedUpdate:
	case ProfilerEventType::CoreVariableUpdate:
	case ProfilerEventType::WorldSystemUpdate:
	case ProfilerEventType::WorldFixedUpdate:
	case ProfilerEventType::WorldVariableUpdate:
		return Colour4f(0.1f, 0.1f, 0.7f);
	case ProfilerEventType::CoreRender:
	case ProfilerEventType::WorldSystemRender:
	case ProfilerEventType::WorldRender:
		return Colour4f(0.7f, 0.1f, 0.1f);
	case ProfilerEventType::PainterDrawCall:
		return Colour4f(0.97f, 0.51f, 0.65f);
	case ProfilerEventType::CoreStartRender:
	case ProfilerEventType::PainterEndRender:
		return Colour4f(1.0f, 0.61f, 0.75f);
	case ProfilerEventType::PainterUpdateProjection:
		return Colour4f(1.0f, 0.71f, 0.85f);
	case ProfilerEventType::StatsView:
		return Colour4f(0.7f, 0.7f, 0.7f);
	case ProfilerEventType::AudioGenerateBuffer:
		return Colour4f(0.5f, 0.8f, 1.0f);
	default:
		return Colour4f(0.1f, 0.7f, 0.1f);
	}
}

void PerformanceStatsView::drawTopSystems(Painter& painter, Rect4f rect)
{
	struct CurEventData {
		const String* name;
		ProfilerEventType type;
		int64_t avg;
		int64_t high;
		int64_t low;
		int64_t highEver;
		int64_t lowEver;

		bool operator< (const CurEventData& other) const
		{
			return high > other.high;
		}
	};

	const auto getTimeLabel = [&] (int64_t t) { return toString((t + 500) / 1000); };

	std::vector<CurEventData> curEvents;
	curEvents.reserve(eventHistory.size());
	for (const auto& [k, v]: eventHistory) {
		curEvents.emplace_back(CurEventData{ &k, v.getType(), v.getAverage(), v.getHighest(), v.getLowest(), v.getHighestEver(), v.getLowestEver() });
	}
	std::sort(curEvents.begin(), curEvents.end());

	std::vector<ColourStringBuilder> columns;
	columns.resize(systemLabels.size());

	columns[0].append("Name:\n");
	columns[1].append("Avg:\n");
	columns[2].append("Loc Min:\n");
	columns[3].append("Loc Max:\n");
	columns[4].append("Loc Var:\n");
	columns[5].append("Min:\n");
	columns[6].append("Max:\n");
	columns[7].append("Var:\n");

	const size_t nToShow = 25;
	for (size_t i = 0; i < nToShow; ++i) {
		const auto& system = curEvents[i];
		columns[0].append(toString(i + 1) + ": ");
		columns[0].append(*system.name + "\n", getEventColour(system.type).inverseMultiplyLuma(0.5f));
		columns[1].append(getTimeLabel(system.avg) + " us\n");
		columns[2].append(getTimeLabel(system.low) + " us\n");
		columns[3].append(getTimeLabel(system.high) + " us\n");
		columns[4].append(getTimeLabel(system.high - system.low) + " us\n");
		columns[5].append(getTimeLabel(system.lowEver) + " us\n");
		columns[6].append(getTimeLabel(system.highEver) + " us\n");
		columns[7].append(getTimeLabel(system.highEver - system.lowEver) + " us\n");
	}

	std::array<float, 8> xPos = { 0, 350, 450, 550, 650, 750, 850, 950 };

	for (size_t i = 1; i < systemLabels.size(); ++i) {
		systemLabels[i].setAlignment(1);
	}

	for (size_t i = 0; i < systemLabels.size(); ++i) {
		auto [str, cols] = columns[i].moveResults();
		systemLabels[i].setText(str).setColourOverride(cols);
		systemLabels[i].setPosition(rect.getTopLeft() + Vector2f(xPos[i], 0)).draw(painter);
	}
}

int64_t PerformanceStatsView::getTimeNs(TimeLine timeline, const ProfilerData& data)
{
	// Total time
	const auto totalTime = data.getTotalElapsedTime();

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
