#pragma once

#include <halley/utils/averaging.h>

#include "stats_view.h"
#include "halley/api/core_api.h"
#include "halley/api/system_api.h"
#include "halley/net/connection/ack_unreliable_connection_stats.h"
#include "halley/support/profiler.h"

namespace Halley
{
	class NetworkSession;
	class INetworkServiceStatsListener;
	class System;

	class PerformanceStatsView : public StatsView, public CoreAPI::IProfileCallback
	{
	public:
		PerformanceStatsView(Resources& resources, const HalleyAPI& api, bool local = true);
		~PerformanceStatsView() override;
		
		void paint(Painter& painter) override;

		void onProfileData(std::shared_ptr<ProfilerData> data) override;
		void setNetworkStats(NetworkSession* networkSession);

		int getNumPages() const;
		int getPageNumber() const;
		void setPageNumber(int page);

        void setPaused(bool paused);
        bool isPaused() const;

		void setDrawBg(bool drawBg);
		void setMousePos(std::optional<Vector2f> mousePos);

	protected:
		bool isInputActive() const override;

	private:
		enum class Page {
			ShortSummary,
			Overall,
			Systems,
			Scripts,
			Audio,
			Network
		};

		class FrameData {
		public:
			int variableTime = 0;
			int fixedTime = 0;
			int renderTime = 0;
		};

		class AudioFrameData {
		public:
			int generateBufferTime = 0;
			int renderVoiceTime = 0;
		};

		class EventHistoryData {
		public:
			EventHistoryData();
			
			void update(ProfilerEventType type, int64_t value, uint64_t sourceId);

			int64_t getMinimum() const;
			int64_t getFirstQuartile() const;
			int64_t getMedian() const;
			int64_t getThirdQuartile() const;
			int64_t getMaximum() const;

			int64_t getHistoricalMinimum() const;
			int64_t getHistoricalMaximum() const;

			int getNumInstances() const;

			ProfilerEventType getType() const;

			void startUpdate();
			bool isVisited() const;

		private:
			ProfilerEventType type;
			Vector<int64_t> samples;
			mutable Vector<int64_t> sortedSamples;
			size_t samplePos = 0;
			int64_t highestEver = 0;
			int64_t lowestEver = std::numeric_limits<int64_t>::max();
			int framesSinceLastVisit = 1;
			int instanceCounter = 0;
			Vector<uint64_t> sources;
			mutable bool needsSorting = false;

			void sortIfNeeded() const;
		};
		
		const bool isLocal;

		TextRenderer headerText;
		TextRenderer fpsLabel;
		TextRenderer graphLabel;
		TextRenderer connLabel;
		TextRenderer toolTipLabel;
		Vector<TextRenderer> systemLabels;

		AveragingLatched<int64_t> totalFrameTime;
		AveragingLatched<int64_t> updateTime;
		AveragingLatched<int64_t> cpuRenderTime;
		AveragingLatched<int64_t> totalRenderTime;
		AveragingLatched<int64_t> vsyncTime;
		AveragingLatched<int64_t> audioTime;
		AveragingLatched<int64_t> gpuTime;
		
		Vector<FrameData> frameData;
		Vector<AudioFrameData> audioFrameData;
		size_t lastFrameData = 0;
		size_t lastAudioFrameData = 0;
		HashMap<String, EventHistoryData> systemHistory;
		HashMap<String, EventHistoryData> scriptHistory;
		HashMap<String, EventHistoryData> audioHistory;
		std::shared_ptr<ProfilerData> lastProfileData;

		float curMaxTime = 500000.0f;
		std::chrono::steady_clock::time_point lastUpdateTime;
		int64_t totalTimePerAudioBuffer = 0;
		int64_t pendingVoiceRenderTime = 0;

		Page page = Page::ShortSummary;
		bool drawBg = true;

		INetworkServiceStatsListener* networkStats = nullptr;
		const NetworkSession* networkSession;

		const Sprite boxBg;
		const Sprite whitebox;

		std::optional<Vector2f> curMousePos;
		std::optional<std::pair<Vector2f, String>> curToolTip;

		bool paused = false;

		void drawHeader(Painter& painter, bool simple);
		void drawFrameTimeline(Painter& painter, Rect4f rect);
		void drawAudioTimeline(Painter& painter, Rect4f rect);
		void drawTimeGraph(Painter& painter, Rect4f rect);
		void drawTimeGraphThreads(Painter& painter, Rect4f rect, Range<ProfilerData::TimePoint> timeRange);
		void drawTimeGraphThread(Painter& painter, Rect4f rect, const ProfilerData::ThreadInfo& threadInfo, Range<ProfilerData::TimePoint> timeRange);
		void drawTopEvents(Painter& painter, Rect4f rect, Time t, const HashMap<String, EventHistoryData>& eventHistory);
		void drawNetworkStats(Painter& painter, Rect4f rect);
		void drawToolTip(Painter& painter, Rect4f rect);
		
		Colour4f getEventColour(ProfilerEventType event) const;
		Colour4f getNetworkStatsCol(const AckUnreliableConnectionStats::PacketStats& stats) const;

		int64_t getTimeNs(TimeLine timeline, const ProfilerData& data);

		void setToolTip(Vector2f pos, String label);
	};
}
