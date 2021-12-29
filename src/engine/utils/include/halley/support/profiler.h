#pragma once

#include "halley/text/halleystring.h"
#include <thread>
#include <gsl/span>
#include <atomic>

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

		PainterDrawCall,
		PainterEndRender,
		PainterUpdateProjection,

		WorldVariableUpdate,
		WorldFixedUpdate,
		WorldRender,
		WorldSystemUpdate,
		WorldSystemRender,

		AudioGenerateBuffer,

		StatsView,

		Game
    };	

    class ProfilerData {
    public:
        using TimePoint = std::chrono::high_resolution_clock::time_point;
    	using Duration = std::chrono::duration<int64_t, std::nano>;
    	
		class Event {
        public:
	        String name;
        	std::thread::id threadId;
			ProfilerEventType type;
			int depth;
        	uint32_t id;
        	TimePoint startTime;
        	TimePoint endTime;
        };

    	class ThreadInfo {
    	public:
    		std::thread::id id;
    		int maxDepth = 0;
    		String name;
    	};

    	ProfilerData() = default;
    	ProfilerData(TimePoint frameStartTime, TimePoint frameEndTime, std::vector<Event> events);

    	TimePoint getStartTime() const;
    	TimePoint getEndTime() const;
    	const std::vector<Event>& getEvents() const;
    	Duration getTotalElapsedTime() const;
		Duration getElapsedTime(ProfilerEventType eventType) const;

    	gsl::span<const ThreadInfo> getThreads() const;

    private:
    	TimePoint frameStartTime;
    	TimePoint frameEndTime;
    	std::vector<Event> events;

    	std::vector<ThreadInfo> threads;

    	void processEvents();
    };
	
    class ProfilerCapture {
    public:
        struct EventId {
	        uint32_t id;
        	uint32_t frameN;
        };
    	
        ProfilerCapture();
    	
    	[[nodiscard]] static ProfilerCapture& get();

    	[[nodiscard]] EventId recordEventStart(ProfilerEventType type, std::string_view name);
    	void recordEventEnd(EventId id);

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

    	std::atomic<bool> recording;
        State state = State::Idle;

    	std::atomic<uint32_t> curId;
    	std::atomic<uint32_t> curFrame;
    	
    	std::chrono::high_resolution_clock::time_point frameStartTime;
    	std::chrono::high_resolution_clock::time_point frameEndTime;

    	std::vector<ProfilerData::Event> events;
    };

	class ProfilerEvent {
	public:
		ProfilerEvent(ProfilerEventType type, std::string_view name = "");
		~ProfilerEvent() noexcept;

		ProfilerEvent(const ProfilerEvent& other) = delete;
		ProfilerEvent(ProfilerEvent&& other) = delete;
		ProfilerEvent& operator=(const ProfilerEvent& other) = delete;
		ProfilerEvent& operator=(ProfilerEvent&& other) = delete;

	private:
		ProfilerCapture::EventId id;
	};
}
