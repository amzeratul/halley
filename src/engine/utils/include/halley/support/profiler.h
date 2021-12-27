#pragma once

#include "halley/text/halleystring.h"
#include <thread>

#include "halley/data_structures/hash_map.h"

namespace Halley {
    class ProfilerData {
    public:
		class Event {
        public:
	        String name;
        	std::thread::id threadId;
        	uint32_t id;
        	std::chrono::high_resolution_clock::time_point startTime;
        	std::chrono::high_resolution_clock::time_point endTime;
        };

    	std::chrono::high_resolution_clock::time_point frameStartTime;
    	std::chrono::high_resolution_clock::time_point frameEndTime;
    	std::vector<Event> events;
    };
	
    class ProfileCapture {
    public:
        ProfileCapture();
    	
    	static ProfileCapture& get();

    	uint32_t recordEventStart(std::string_view name);
    	void recordEventEnd(uint32_t id);

    	bool isRecording() const;

    	void startFrame(bool record, size_t maxEvents = 10240);
    	ProfilerData endFrame();

    private:
    	bool recording = false;
    	std::atomic<uint32_t> curId;
    	std::chrono::high_resolution_clock::time_point frameStartTime;
    	std::vector<ProfilerData::Event> events;
    };

	class ProfileEvent {
	public:
		ProfileEvent(std::string_view name);
		~ProfileEvent() noexcept;

	private:
		uint32_t id;
	};
}
