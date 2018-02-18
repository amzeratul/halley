#include "halley/net/devcon/devcon_messages.h"
#include "halley/text/halleystring.h"
#include "halley/file/byte_serializer.h"
using namespace Halley;
using namespace DevCon;

void DevCon::setupMessageQueue(MessageQueue& queue)
{
	queue.setChannel(0, ChannelSettings(true, true));

	queue.addFactory<LogMsg>();
}

LogMsg::LogMsg(gsl::span<const gsl::byte> data)
{
	Deserializer s(data);
	s >> level;
	s >> msg;
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
