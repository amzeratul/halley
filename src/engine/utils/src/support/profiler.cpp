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
	if (recording) {
		const auto id = curId++;
		const auto threadId = std::this_thread::get_id();
		const auto time = std::chrono::high_resolution_clock::now();
		events[id] = ProfilerData::Event{ name, threadId, type, id, time, {} };
		return id;
	} else {
		return std::numeric_limits<uint32_t>::max();
	}
}

void ProfileCapture::recordEventEnd(uint32_t id)
{
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
	}
	events.clear();

	if (rec) {
		events.resize(maxFrames);
	}

	state = State::FrameStarted;
}

std::optional<ProfilerData> ProfileCapture::endFrame(bool capture)
{
	Expects(state == State::FrameStarted);
	
	const auto now = std::chrono::high_resolution_clock::now();

	std::optional<ProfilerData> result;
	if (capture) {
		result = ProfilerData{ frameStartTime, now, std::move(events) };
		events.clear();
	}

	frameStartTime = now;
	state = State::FrameEnded;

	return result;
}


ProfileEvent::ProfileEvent(ProfilerEventType type, std::string_view name)
	: id(ProfileCapture::get().recordEventStart(type, name))
{
}

ProfileEvent::~ProfileEvent() noexcept
{
	ProfileCapture::get().recordEventEnd(id);
}
