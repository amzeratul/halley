#pragma once

#include "halley/data_structures/config_node.h"

namespace Halley {
	class TimelineKeyFrame {
	public:
		TimelineKeyFrame() = default;
		TimelineKeyFrame(int frameNumber, ConfigNode data);
		TimelineKeyFrame(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		int getFrameNumber() const;
		const ConfigNode& getData() const;
		void setData(ConfigNode data);

		bool operator<(const TimelineKeyFrame& other) const;

	private:
		int frameNumber;
		ConfigNode data;
	};

    class TimelineSequence {
    public:
		struct Key {
			String groupId;
			String keyId;
			String subKeyId;

			Key() = default;
			Key(String groupId, String keyId = "", String subKeyId = "");
			Key(const ConfigNode& node);

			ConfigNode toConfigNode() const;

			bool operator==(const Key& other) const;
			bool operator!=(const Key& other) const;
		};

    	TimelineSequence() = default;
		explicit TimelineSequence(Key key);
        TimelineSequence(const ConfigNode& node);

        ConfigNode toConfigNode() const;

		const Key& getKey() const;

		void addKeyFrame(int frameNumber, ConfigNode data);

	private:
		Key key;
		Vector<TimelineKeyFrame> keyFrames;
    };

	class TimelineSequenceEntity {
	public:
		String entityId;
		Vector<TimelineSequence> sequences;

		TimelineSequenceEntity(String entityId = {});
		TimelineSequenceEntity(const ConfigNode& node);

		ConfigNode toConfigNode() const;
	};
}
