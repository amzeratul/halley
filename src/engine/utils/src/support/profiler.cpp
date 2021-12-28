#include "halley/support/profiler.h"

#include "halley/utils/algorithm.h"

using namespace Halley;


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
	};
	HashMap<std::thread::id, ThreadCurInfo> threadInfo;

	// Process each event
	for (auto& e: events) {
		auto& curThread = threadInfo[e.threadId];

		// If this event starts after the end of the previous stack, then it's not nested in it, pop previous.
		// Repeat for as many levels as needed, up to the root
		while (!curThread.stackEnds.empty() && e.startTime >= curThread.stackEnds.back()) {
			curThread.stackEnds.pop_back();
		}
		const auto depth = curThread.stackEnds.size();
		curThread.maxDepth = std::max(curThread.maxDepth, depth);
		e.depth = static_cast<int>(depth);
		curThread.stackEnds.push_back(e.endTime);
	}

	// Generate the thread list
	for (const auto& [k, v]: threadInfo) {
		const String name; // TODO
		threads.emplace_back(ThreadInfo{ k, static_cast<int>(v.maxDepth), name });
	}
}

ProfileCapture::ProfileCapture()
	: curId(0)
{
}

ProfileCapture& ProfileCapture::get()
{
	// TODO: move to HalleyStatics?
	static ProfileCapture profiler;
	return profiler;
}

uint32_t ProfileCapture::recordEventStart(ProfilerEventType type, std::string_view name)
{
	Expects(state == State::FrameStarted);
	
	if (recording) {
		const auto id = curId++;
		if (id < events.size()) {
			const auto threadId = std::this_thread::get_id();
			const auto time = std::chrono::high_resolution_clock::now();
			events[id] = ProfilerData::Event{ name, threadId, type, 0, id, time, {} };
			return id;
		}
	}
	return std::numeric_limits<uint32_t>::max();
}

void ProfileCapture::recordEventEnd(uint32_t id)
{
	Expects(state == State::FrameStarted);
	
	if (recording && id != std::numeric_limits<uint32_t>::max()) {
		const auto time = std::chrono::high_resolution_clock::now();
		events[id].endTime = time;
	}
}

bool ProfileCapture::isRecording() const
{
	return recording;
}

void ProfileCapture::startFrame(bool rec, size_t maxFrames)
{
	Expects(state != State::FrameStarted);
	
	recording = rec;
	if (state == State::Idle) {
		frameStartTime = std::chrono::high_resolution_clock::now();
	} else {
		frameStartTime = frameEndTime;
	}
	frameEndTime = {};
	events.clear();
	curId = 0;

	if (rec) {
		events.resize(maxFrames);
	}

	state = State::FrameStarted;
}

void ProfileCapture::endFrame()
{
	Expects(state == State::FrameStarted);
	
	frameEndTime = std::chrono::high_resolution_clock::now();
	state = State::FrameEnded;
}

ProfilerData ProfileCapture::getCapture()
{
	Expects(state == State::FrameEnded);

	events.resize(curId);
	return ProfilerData(frameStartTime, frameEndTime, std::move(events));
}

Time ProfileCapture::getFrameTime() const
{
	Expects(state == State::FrameEnded);
	
	return std::chrono::duration<Time>(frameEndTime - frameStartTime).count();
}


ProfileEvent::ProfileEvent(ProfilerEventType type, std::string_view name)
	: id(ProfileCapture::get().recordEventStart(type, name))
{
}

ProfileEvent::~ProfileEvent() noexcept
{
	ProfileCapture::get().recordEventEnd(id);
}
