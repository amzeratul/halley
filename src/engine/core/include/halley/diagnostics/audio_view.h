#pragma once

#include "stats_view.h"
#include "halley/api/audio_api.h"

namespace Halley
{
	class AudioView : public StatsView, public IAudioDebugDataListener
	{
	public:
		AudioView(Resources& resources, const HalleyAPI& api);
		~AudioView() override;

		void update(Time t) override;
		void paint(Painter& painter) override;

	protected:
		void onAudioDebugData(AudioDebugData data) override;

	private:
		Sprite boxBg;
		Sprite whitebox;
		TextRenderer headerText;

		bool listenerRegistered = false;
		bool populatedObjectNames = false;
		AudioDebugData lastData;
		AudioDebugData curData;

		Mutex mutex;

		mutable HashMap<AudioEmitterId, String> emitterNames;
		mutable HashMap<AudioObjectId, String> objectNames;
		mutable HashMap<AudioRegionId, String> regionNames;
		mutable Vector<AudioEmitterId> emittersThatNeedName;
		mutable Vector<AudioEmitterId> objectsThatNeedName;
		mutable Vector<AudioEmitterId> regionsThatNeedName;

		String getEmitterName(AudioEmitterId emitterId) const;
		String getRegionName(AudioRegionId regionId) const;
		String getObjectName(AudioObjectId objectId) const;

		String getMixString(const AudioDebugData::VoiceData& data) const;
	};
}
