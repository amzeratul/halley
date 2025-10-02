#pragma once

#include "halley/text/halleystring.h"
#include <thread>
#include <gsl/span>
#include <atomic>

#include "halley/api/halley_api.h"
#include "halley/data_structures/config_node.h"
#include "halley/data_structures/hash_map.h"
#include "halley/time/halleytime.h"

namespace Halley {
	enum class ProfilerEventType : uint8_t {
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
		WorldSystemMessages,

        ScriptUpdate,

		AudioGenerateBuffer,
		AudioRenderVoice,

        GPU,

		DiskIO,

		StatsView,

		Game,

        ExternalCode,
        UserDefined
    };

	template <>
	struct EnumNames<ProfilerEventType> {
		constexpr std::array<const char*, 29> operator()() const {
			return{{
			    "CorePumpEvents",
				"CoreDevConClient",
				"CorePumpAudio",
				"CoreFixedUpdate",
				"CoreVariableUpdate",
				"CoreUpdateSystem",
				"CoreUpdatePlatform",
				"CoreUpdate",
				"CoreStartRender",
				"CoreRender",
				"CoreVSync",

				"PainterDrawCall",
				"PainterEndRender",
				"PainterUpdateProjection",

				"WorldVariableUpdate",
				"WorldFixedUpdate",
				"WorldRender",
				"WorldSystemUpdate",
				"WorldSystemRender",
				"WorldSystemMessages",

		        "ScriptUpdate",

				"AudioGenerateBuffer",
				"AudioRenderVoice",

		        "GPU",

				"DiskIO",

				"StatsView",

				"Game",

		        "ExternalCode",
		        "UserDefined",
			}};
		}
	};

    class ProfilerData {
    public:
        using TimePoint = int64_t;// std::chrono::steady_clock::time_point;
        using Duration = int64_t;// std::chrono::duration<int64_t, std::nano>;

        enum class ThreadType {
	        Update,
            Render,
            GPU,
            Audio,
            Network,
            Misc
        };

		class Event {
        public:
	        String name;
			ProfilerEventType type;
			int16_t depth;
        	uint64_t id;
			uint64_t sourceId;
        	TimePoint startTime;
        	TimePoint endTime;

            // Non-serializable
			std::thread::id threadId;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
        };

    	class ThreadInfo {
    	public:
    		int maxDepth = 0;
    		String name;
    		TimePoint startTime;
    		TimePoint endTime;
    		Duration totalTime;
            ThreadType type;

            Vector<Event> events;

    		bool operator< (const ThreadInfo& other) const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
    	};

    	ProfilerData() = default;
    	ProfilerData(TimePoint frameStartTime, TimePoint frameEndTime, Vector<Event> events);
        void collectAPIData(const HalleyAPI& api, std::optional<SystemAPI::MemoryUsage> memoryUsage);

    	TimePoint getStartTime() const;
    	TimePoint getEndTime() const;
    	Duration getTotalElapsedTime() const;
		Duration getElapsedTime(ProfilerEventType eventType) const;
		Duration getElapsedTime(gsl::span<const ProfilerEventType> eventTypes) const;

    	gsl::span<const ThreadInfo> getThreads() const;
        bool getHasVsync() const;
        int64_t getAudioTime() const;
        uint32_t getAudioSampleRate() const;
        uint32_t getAudioBufferLen() const;
        SystemAPI::MemoryUsage getMemoryUsage() const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

    private:
    	TimePoint frameStartTime;
    	TimePoint frameEndTime;

    	Vector<ThreadInfo> threads;

        bool hasVsync = false;
        int64_t audioTime = 0;
        uint32_t audioSampleRate = 0;
        uint32_t audioBufferLen = 0;
        SystemAPI::MemoryUsage memoryUsage;

    	void processEvents(Vector<Event> pendingEvents);
    };
	
    class ProfilerCapture {
    public:
        using EventId = uint64_t;
    	
        ProfilerCapture(size_t maxEvents = 16384);
    	
    	[[nodiscard]] static ProfilerCapture& get();

    	[[nodiscard]] EventId recordEventStart(ProfilerEventType type, std::string_view name, uint64_t sourceId);
    	void recordEventEnd(EventId id);
    	[[nodiscard]] EventId recordEventStart(ProfilerEventType type, std::string_view name, uint64_t sourceId, std::chrono::steady_clock::time_point time);
    	void recordEventEnd(EventId id, std::chrono::steady_clock::time_point time);

    	[[nodiscard]] bool isRecording() const;

    	void startFrame(bool record, Time dt);
    	void endFrame();
		ProfilerData getCapture(const HalleyAPI& api);

    	Time getFrameTime() const;

    private:
    	enum class State {
    		Idle,
    		FrameStarted,
    		FrameEnded
    	};

    	std::atomic<bool> recording;
    	std::atomic<uint64_t> curId;
    	uint64_t startId;
    	std::atomic<uint64_t> endId;
        State state = State::Idle;
    	
    	std::chrono::steady_clock::time_point frameStartTime;
    	std::chrono::steady_clock::time_point frameEndTime;

    	Vector<ProfilerData::Event> events;

		std::optional<SystemAPI::MemoryUsage> memoryUsage;
		Time memoryUsageRefreshTime = 1;
    };

	class ProfilerEvent {
	public:
		ProfilerEvent(ProfilerEventType type, std::string_view name = "", uint64_t sourceId = 0);
		~ProfilerEvent() noexcept;

		ProfilerEvent(const ProfilerEvent& other) = delete;
		ProfilerEvent(ProfilerEvent&& other) = delete;
		ProfilerEvent& operator=(const ProfilerEvent& other) = delete;
		ProfilerEvent& operator=(ProfilerEvent&& other) = delete;

	private:
		ProfilerCapture::EventId id;
	};
}
