#include "halley/support/profiler.h"

#include "halley/utils/algorithm.h"
#include "halley/bytes/byte_serializer.h"

using namespace Halley;


bool ProfilerData::ThreadInfo::operator<(const ThreadInfo& other) const
{
	if (type != other.type) {
		return type < other.type;
	}
	return totalTime > other.totalTime;
}

ProfilerData::ProfilerData(TimePoint frameStartTime, TimePoint frameEndTime, Vector<Event> events)
	: frameStartTime(frameStartTime)
	, frameEndTime(frameEndTime)
{
	processEvents(std::move(events));
}

ProfilerData::TimePoint ProfilerData::getStartTime() const
{
	return frameStartTime;
}

ProfilerData::TimePoint ProfilerData::getEndTime() const
{
	return frameEndTime;
}

ProfilerData::Duration ProfilerData::getTotalElapsedTime() const
{
	return frameEndTime - frameStartTime;
}

ProfilerData::Duration ProfilerData::getElapsedTime(ProfilerEventType eventType) const
{
	return getElapsedTime(gsl::span<const ProfilerEventType>(&eventType, 1));
}

ProfilerData::Duration ProfilerData::getElapsedTime(gsl::span<const ProfilerEventType> eventTypes) const
{
	TimePoint start = {};
	TimePoint end = {};
	bool first = true;

	for (const auto& t : threads) {
		for (const auto& e : t.events) {
			if (std_ex::contains(eventTypes, e.type)) {
				if (first) {
					start = e.startTime;
					first = false;
				}
				end = e.endTime;
			}
		}
	}

	return end - start;
}

gsl::span<const ProfilerData::ThreadInfo> ProfilerData::getThreads() const
{
	return threads;
}

void ProfilerData::processEvents(Vector<Event> pendingEvents)
{
	struct ThreadCurInfo {
		size_t maxDepth = 0;
		Vector<TimePoint> stackEnds;
		TimePoint start;
		TimePoint end;
		Duration totalTime;
		bool first = true;
		ThreadType type = ThreadType::Misc;
		Vector<Event> events;
	};
	HashMap<std::thread::id, ThreadCurInfo> threadInfo;

	// Process each event
	for (auto& pe: pendingEvents) {
		auto& curThread = threadInfo[pe.threadId];
		auto& e = curThread.events.emplace_back(std::move(pe));

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
		e.depth = static_cast<int16_t>(depth);
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

		// Process special events
		if (e.type == ProfilerEventType::CoreVariableUpdate) {
			curThread.type = ThreadType::Update;
		} else if (e.type == ProfilerEventType::CoreRender) {
			curThread.type = ThreadType::Render;
		} else if (e.type == ProfilerEventType::AudioGenerateBuffer) {
			curThread.type = ThreadType::Audio;
		} else if (e.type == ProfilerEventType::GPU) {
			curThread.type = ThreadType::GPU;
		}
	}

	// Generate the thread list
	for (const auto& [k, v]: threadInfo) {
		const String name; // TODO
		threads.emplace_back(ThreadInfo{ static_cast<int>(v.maxDepth), name, v.start, v.end, v.totalTime, v.type, std::move(v.events) });
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
	return recordEventStart(type, name, std::chrono::steady_clock::now());
}

void ProfilerCapture::recordEventEnd(EventId id)
{
	recordEventEnd(id, std::chrono::steady_clock::now());
}

ProfilerCapture::EventId ProfilerCapture::recordEventStart(ProfilerEventType type, std::string_view name, std::chrono::steady_clock::time_point time)
{
	if (recording) {
		const auto id = curId++; // I think this is thread-safe?
		if (id < endId) {
			const auto pos = id % events.size();
			const auto threadId = type == ProfilerEventType::GPU ? std::thread::id() : std::this_thread::get_id();
			events[pos] = ProfilerData::Event{ name, type, 0, id, time.time_since_epoch().count(), {}, threadId };
			return id;
		} else {
			--curId;
		}
	}
	return 0;
}

void ProfilerCapture::recordEventEnd(EventId id, std::chrono::steady_clock::time_point time)
{
	if (recording && id != 0) {
		const auto pos = id % events.size();
		if (events[pos].id == id) { // Dodgy, potential race condition here
			events[pos].endTime = time.time_since_epoch().count();
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

	Vector<ProfilerData::Event> eventsCopy;
	if (startIdx < endIdx) {
		eventsCopy.insert(eventsCopy.end(), events.begin() + startIdx, events.begin() + endIdx);
	} else {
		eventsCopy.insert(eventsCopy.end(), events.begin() + startIdx, events.end());
		eventsCopy.insert(eventsCopy.end(), events.begin(), events.begin() + endIdx);
	}
	
	return ProfilerData(frameStartTime.time_since_epoch().count(), frameEndTime.time_since_epoch().count(), std::move(eventsCopy));
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
	case ProfilerEventType::GPU:
		return true;
	default:
		return false;
	}
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

void ProfilerData::Event::serialize(Serializer& s) const
{
	s << name;
	s << type;
	s << depth;
	s << id;
	s << startTime;
	s << endTime;
}

void ProfilerData::Event::deserialize(Deserializer& s)
{
	s >> name;
	s >> type;
	s >> depth;
	s >> id;
	s >> startTime;
	s >> endTime;
}

void ProfilerData::ThreadInfo::serialize(Serializer& s) const
{
    s << maxDepth;
    s << name;
    s << startTime;
    s << endTime;
    s << totalTime;
    s << type;
	s << events;
}

void ProfilerData::ThreadInfo::deserialize(Deserializer& s)
{
    s >> maxDepth;
    s >> name;
    s >> startTime;
    s >> endTime;
    s >> totalTime;
    s >> type;
	s >> events;
}

void ProfilerData::serialize(Serializer& s) const
{
	s << frameStartTime;
    s << frameEndTime;
    s << threads;
}

void ProfilerData::deserialize(Deserializer& s)
{
	s >> frameStartTime;
    s >> frameEndTime;
    s >> threads;
}
