#include "halley/core/devcon/devcon_messages.h"
#include "halley/text/halleystring.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/net/connection/message_queue.h"

using namespace Halley;
using namespace DevCon;

void DevCon::setupMessageQueue(MessageQueue& queue)
{
	queue.setChannel(0, ChannelSettings(true, true));

	queue.addFactory<LogMsg>();
	queue.addFactory<ReloadAssetsMsg>();
}


LogMsg::LogMsg(LoggerLevel level, const String& msg)
	: level(level)
	, msg(msg)
{}

void LogMsg::serialize(Serializer& s) const
{
	s << level;
	s << msg;
}

void LogMsg::deserialize(Deserializer& s)
{
	s >> level;
	s >> msg;
}

LoggerLevel LogMsg::getLevel() const
{
	return level;
}

const String& LogMsg::getMessage() const
{
	return msg;
}

MessageType LogMsg::getMessageType() const
{
	return MessageType::Log;
}


ReloadAssetsMsg::ReloadAssetsMsg(gsl::span<const String> ids)
	: ids(ids.begin(), ids.end())
{}

void ReloadAssetsMsg::serialize(Serializer& s) const
{
	s << ids;
}

void ReloadAssetsMsg::deserialize(Deserializer& s)
{
	s >> ids;
}

Vector<String> ReloadAssetsMsg::getIds() const
{
	return ids;
}

MessageType ReloadAssetsMsg::getMessageType() const
{
	return MessageType::ReloadAssets;
}
