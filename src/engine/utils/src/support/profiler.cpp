#include "halley/support/profiler.h"

#include "halley/utils/algorithm.h"

using namespace Halley;


bool ProfilerData::ThreadInfo::operator<(const ThreadInfo& other) const
{
	return totalTime > other.totalTime;
}

ProfilerData::ProfilerData(TimePoint frameStartTime, TimePoint frameEndTime, std::vector<Event> events)
	: frameStartTime(frameStartTime)
	, frameEndTime(frameEndTime)
	, events(std::move(events))
{
	processEvents();
}

ProfilerData::TimePoint ProfilerData::getStartTime() const
{
	return frameStartTime;
}

ProfilerData::TimePoint ProfilerData::getEndTime() const
{
	return frameEndTime;
}

const std::vector<ProfilerData::Event>& ProfilerData::getEvents() const
{
	return events;
}

ProfilerData::Duration ProfilerData::getTotalElapsedTime() const
{
	return frameEndTime - frameStartTime;
}

ProfilerData::Duration ProfilerData::getElapsedTime(ProfilerEventType eventType) const
{
	TimePoint start = {};
	TimePoint end = {};
	bool first = true;
	
	for (const auto& e: events) {
		if (e.type == eventType) {
			if (first) {
				start = e.startTime;
				first = false;
			}
			end = e.endTime;
		}
	}

	return end - start;
}

gsl::span<const ProfilerData::ThreadInfo> ProfilerData::getThreads() const
{
	return threads;
}

void ProfilerData::processEvents()
{
	struct ThreadCurInfo {
		size_t maxDepth = 0;
		std::vector<TimePoint> stackEnds;
		TimePoint start;
		TimePoint end;
		Duration totalTime;
		bool first = true;
	};
	HashMap<std::thread::id, ThreadCurInfo> threadInfo;

	// Process each event
	for (auto& e: events) {
		auto& curThread = threadInfo[e.threadId];

		// Normalize ends
		if (e.startTime == TimePoint{}) {
			e.startTime = frameStartTime;
		}
		if (e.endTime == TimePoint{}) {
			e.endTime = frameEndTime;
		}

		// Compute depth
		// If this event starts after the end of the previous stack, then it's not nested in it, pop previous.
		// Repeat for as many levels as needed, up to the root
		while (!curThread.stackEnds.empty() && e.startTime >= curThread.stackEnds.back()) {
			curThread.stackEnds.pop_back();
		}
		const auto depth = curThread.stackEnds.size();
		curThread.maxDepth = std::max(curThread.maxDepth, depth);
		e.depth = static_cast<int>(depth);
		curThread.stackEnds.push_back(e.endTime);

		// Store timing
		if (curThread.first) {
			curThread.start = e.startTime;
		} else {
			curThread.start = std::min(curThread.start, e.startTime);
		}
		curThread.end = std::max(curThread.end, e.endTime);
		if (e.type != ProfilerEventType::CoreVSync && e.depth == 0) {
			curThread.totalTime += e.endTime - e.startTime;
		}
	}

	// Generate the thread list
	for (const auto& [k, v]: threadInfo) {
		const String name; // TODO
		threads.emplace_back(ThreadInfo{ k, static_cast<int>(v.maxDepth), name, v.start, v.end, v.totalTime });
	}
	std::sort(threads.begin(), threads.end());
}

ProfilerCapture::ProfilerCapture(size_t maxEvents)
	: recording(false)
	, curId(0)
{
	events.resize(maxEvents);
}

ProfilerCapture& ProfilerCapture::get()
{
	// TODO: move to HalleyStatics?
	static ProfilerCapture profiler;
	return profiler;
}

ProfilerCapture::EventId ProfilerCapture::recordEventStart(ProfilerEventType type, std::string_view name)
{
	if (recording) {
		const auto id = curId++; // I think this is thread-safe?
		if (id < endId) {
			const auto pos = id % events.size();
			const auto threadId = std::this_thread::get_id();
			const auto time = std::chrono::steady_clock::now();
			events[pos] = ProfilerData::Event{ name, threadId, type, 0, id, time, {} };
			return id;
		} else {
			--curId;
		}
	}
	return 0;
}

void ProfilerCapture::recordEventEnd(EventId id)
{
	if (recording && id != 0) {
		const auto time = std::chrono::steady_clock::now();
		const auto pos = id % events.size();
		if (events[pos].id == id) { // Dodgy, potential race condition here
			events[pos].endTime = time;
		}
	}
}

bool ProfilerCapture::isRecording() const
{
	return recording;
}

void ProfilerCapture::startFrame(bool rec)
{
	Expects(state != State::FrameStarted);
	
	if (state == State::Idle) {
		frameStartTime = std::chrono::steady_clock::now();
	} else {
		frameStartTime = frameEndTime;
	}
	frameEndTime = {};

	startId = curId.fetch_add(events.size()) + events.size();
	endId = startId + events.size();

	recording = rec;
	state = State::FrameStarted;
}

void ProfilerCapture::endFrame()
{
	Expects(state == State::FrameStarted);
	
	frameEndTime = std::chrono::steady_clock::now();
	state = State::FrameEnded;
}

ProfilerData ProfilerCapture::getCapture()
{
	Expects(state == State::FrameEnded);

	const auto startIdx = startId % events.size();
	const auto endIdx = curId % events.size();

	std::vector<ProfilerData::Event> eventsCopy;
	if (startIdx < endIdx) {
		eventsCopy.insert(eventsCopy.end(), events.begin() + startIdx, events.begin() + endIdx);
	} else {
		eventsCopy.insert(eventsCopy.end(), events.begin() + startIdx, events.end());
		eventsCopy.insert(eventsCopy.end(), events.begin(), events.begin() + endIdx);
	}
	
	return ProfilerData(frameStartTime, frameEndTime, std::move(eventsCopy));
}

Time ProfilerCapture::getFrameTime() const
{
	Expects(state == State::FrameEnded);
	
	return std::chrono::duration<Time>(frameEndTime - frameStartTime).count();
}

constexpr static bool isDevMode()
{
#ifdef DEV_BUILD
	return true;
#else
	return false;
#endif
}

constexpr static bool alwaysLogType(ProfilerEventType type)
{
	switch (type) {
	case ProfilerEventType::CoreRender:
	case ProfilerEventType::CoreVSync:
	case ProfilerEventType::CoreFixedUpdate:
	case ProfilerEventType::CoreVariableUpdate:
		return true;
	}
	return false;
}

ProfilerEvent::ProfilerEvent(ProfilerEventType type, std::string_view name)
{
	if (isDevMode() || alwaysLogType(type)) {
		id = ProfilerCapture::get().recordEventStart(type, name);
	}
}

ProfilerEvent::~ProfilerEvent() noexcept
{
	if (id != 0) {
		ProfilerCapture::get().recordEventEnd(id);
	}
}
