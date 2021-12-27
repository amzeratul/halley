#pragma once

#include "halley/text/halleystring.h"
#include <thread>

#include "halley/data_structures/hash_map.h"
#include "halley/time/halleytime.h"

namespace Halley {
	enum class ProfilerEventType {
	    CorePumpEvents,
		CoreDevConClient,
		CorePumpAudio,
		CoreFixedUpdate,
		CoreVariableUpdate,
		CoreUpdateSystem,
		CoreUpdatePlatform,
		CoreUpdate,
		CoreStartRender,
		CoreRender,
		CoreVSync,

		WorldVariableUpdate,
		WorldFixedUpdate,
		WorldRender,
		WorldSystemUpdate,
		WorldSystemRender,

		Game
    };	

    class ProfilerData {
    public:
		class Event {
        public:
	        String name;
        	std::thread::id threadId;
			ProfilerEventType type;
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
    	
    	[[nodiscard]] static ProfileCapture& get();

    	[[nodiscard]] uint32_t recordEventStart(ProfilerEventType type, std::string_view name);
    	void recordEventEnd(uint32_t id);

    	[[nodiscard]] bool isRecording() const;

    	void startFrame(bool record, size_t maxEvents = 10240);
    	void endFrame();
		ProfilerData getCapture();

    	Time getFrameTime() const;

    private:
    	enum class State {
    		Idle,
    		FrameStarted,
    		FrameEnded
    	};

    	bool recording = false;
    	std::atomic<uint32_t> curId;
    	std::chrono::high_resolution_clock::time_point frameStartTime;
    	std::chrono::high_resolution_clock::time_point frameEndTime;
    	std::vector<ProfilerData::Event> events;
        State state = State::Idle;
    };

	class ProfileEvent {
	public:
		ProfileEvent(ProfilerEventType type, std::string_view name = "");
		~ProfileEvent() noexcept;

		ProfileEvent(const ProfileEvent& other) = delete;
		ProfileEvent(ProfileEvent&& other) = delete;
		ProfileEvent& operator=(const ProfileEvent& other) = delete;
		ProfileEvent& operator=(ProfileEvent&& other) = delete;

	private:
		uint32_t id;
	};
}
