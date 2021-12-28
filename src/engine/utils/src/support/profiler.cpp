#include "halley/support/profiler.h"

using namespace Halley;


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
			events[id] = ProfilerData::Event{ name, threadId, type, id, time, {} };
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
	
	return ProfilerData{ frameStartTime, frameEndTime, std::move(events) };
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
