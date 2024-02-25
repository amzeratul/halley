#include "halley/timeline/timeline_sequence.h"

#include "halley/utils/algorithm.h"

using namespace Halley;

TimelineKeyFrame::TimelineKeyFrame(int frameNumber, ConfigNode data)
	: frameNumber(frameNumber)
	, data(std::move(data))
{
}

TimelineKeyFrame::TimelineKeyFrame(const ConfigNode& node)
{
	frameNumber = node["frameNumber"].asInt();
	data = ConfigNode(node["data"]);
}

ConfigNode TimelineKeyFrame::toConfigNode() const
{
	ConfigNode::MapType result;
	result["frameNumber"] = frameNumber;
	result["data"] = ConfigNode(data);
	return result;
}

int TimelineKeyFrame::getFrameNumber() const
{
	return frameNumber;
}

const ConfigNode& TimelineKeyFrame::getData() const
{
	return data;
}

void TimelineKeyFrame::setData(ConfigNode data)
{
	this->data = std::move(data);
}

bool TimelineKeyFrame::operator<(const TimelineKeyFrame& other) const
{
	return frameNumber < other.frameNumber;
}

TimelineSequence::Key::Key(String groupId, String keyId, String subKeyId)
	: groupId(std::move(groupId))
    , keyId(std::move(keyId))
    , subKeyId(std::move(subKeyId))
{}

TimelineSequence::Key::Key(const ConfigNode& node)
{
	groupId = node["groupId"].asString("");
	keyId = node["keyId"].asString("");
	subKeyId = node["subKeyId"].asString("");
}

ConfigNode TimelineSequence::Key::toConfigNode() const
{
	ConfigNode::MapType result;
	if (!groupId.isEmpty()) {
		result["groupId"] = groupId;
	}
	if (!keyId.isEmpty()) {
		result["keyId"] = keyId;
	}
	if (!subKeyId.isEmpty()) {
		result["subKeyId"] = subKeyId;
	}
	return result;
}

bool TimelineSequence::Key::operator==(const Key& other) const
{
	return groupId == other.groupId && keyId == other.keyId && subKeyId == other.subKeyId;
}

bool TimelineSequence::Key::operator!=(const Key& other) const
{
	return !(*this == other);
}

TimelineSequence::TimelineSequence(Key key)
	: key(std::move(key))
{
}

TimelineSequence::TimelineSequence(const ConfigNode& node)
{
	key = node["key"];
	keyFrames = node["keyFrames"].asVector<TimelineKeyFrame>({});
}

ConfigNode TimelineSequence::toConfigNode() const
{
	ConfigNode::MapType result;
	result["key"] = key;
	result["keyFrames"] = keyFrames;
	return result;
}

const TimelineSequence::Key& TimelineSequence::getKey() const
{
	return key;
}

void TimelineSequence::addKeyFrame(int frameNumber, ConfigNode data)
{
	const auto iter = std_ex::find_if(keyFrames, [&] (const auto& e) { return e.getFrameNumber() == frameNumber; });
	if (iter != keyFrames.end()) {
		iter->setData(std::move(data));
	} else {
		keyFrames.emplace_back(frameNumber, std::move(data));
		std::sort(keyFrames.begin(), keyFrames.end());
	}
}

TimelineSequenceEntity::TimelineSequenceEntity(String entityId)
	: entityId(std::move(entityId))
{
}

TimelineSequenceEntity::TimelineSequenceEntity(const ConfigNode& node)
{
	entityId = node["entityId"].asString();
	sequences = node["sequences"].asVector<TimelineSequence>({});
}

ConfigNode TimelineSequenceEntity::toConfigNode() const
{
	ConfigNode::MapType result;
	result["entityId"] = entityId;
	result["sequences"] = sequences;
	return result;
}
