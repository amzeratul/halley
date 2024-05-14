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

		std::mutex mutex;

		mutable HashMap<AudioEmitterId, String> emitterNames;
		mutable HashMap<AudioObjectId, String> objectNames;

		String getName(AudioEmitterId emitterId) const;
		String getObjectName(AudioObjectId objectId) const;
	};
}
