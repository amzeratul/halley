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

uint32_t ProfileCapture::recordEventStart(std::string_view name)
{
	if (recording) {
		const auto id = curId++;
		const auto threadId = std::this_thread::get_id();
		const auto time = std::chrono::high_resolution_clock::now();
		events[id] = ProfilerData::Event{ name, threadId, id, time, {} };
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
	recording = rec;
	frameStartTime = std::chrono::high_resolution_clock::now();
	events.clear();

	if (rec) {
		events.resize(maxFrames);
	}
}

ProfilerData ProfileCapture::endFrame()
{
	ProfilerData result;
	result.frameStartTime = frameStartTime;
	result.frameEndTime = std::chrono::high_resolution_clock::now();
	result.events = std::move(events);
	events.clear();
	return result;
}


ProfileEvent::ProfileEvent(std::string_view name)
	: id(ProfileCapture::get().recordEventStart(name))
{
}

ProfileEvent::~ProfileEvent() noexcept
{
	ProfileCapture::get().recordEventEnd(id);
}
