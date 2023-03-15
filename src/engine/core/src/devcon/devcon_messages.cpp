#include "halley/devcon/devcon_messages.h"
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
	queue.addFactory<RegisterInterestMsg>();
	queue.addFactory<UpdateInterestMsg>();
	queue.addFactory<UnregisterInterestMsg>();
	queue.addFactory<NotifyInterestMsg>();
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

RegisterInterestMsg::RegisterInterestMsg(String id, ConfigNode params, uint32_t handle)
	: id(std::move(id))
	, params(std::move(params))
	, handle(handle)
{
}

void RegisterInterestMsg::serialize(Serializer& s) const
{
	s << id;
	s << params;
	s << handle;
}

void RegisterInterestMsg::deserialize(Deserializer& s)
{
	s >> id;
	s >> params;
	s >> handle;
}


UpdateInterestMsg::UpdateInterestMsg(uint32_t handle, ConfigNode params)
	: handle(handle)
	, params(std::move(params))
{
}

void UpdateInterestMsg::serialize(Serializer& s) const
{
	s << handle;
	s << params;
}

void UpdateInterestMsg::deserialize(Deserializer& s)
{
	s >> handle;
	s >> params;
}


UnregisterInterestMsg::UnregisterInterestMsg(uint32_t handle)
	: handle(handle)
{}

void UnregisterInterestMsg::serialize(Serializer& s) const
{
	s << handle;
}

void UnregisterInterestMsg::deserialize(Deserializer& s)
{
	s >> handle;
}

NotifyInterestMsg::NotifyInterestMsg(uint32_t handle, ConfigNode data)
	: handle(handle)
	, data(std::move(data))
{
}

void NotifyInterestMsg::serialize(Serializer& s) const
{
	s << handle;
	s << data;
}

void NotifyInterestMsg::deserialize(Deserializer& s)
{
	s >> handle;
	s >> data;
}
