#include "halley/devcon/devcon_messages.h"
#include "halley/text/halleystring.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/net/connection/message_queue.h"

using namespace Halley;
using namespace DevCon;


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


ReloadAssetsMsg::ReloadAssetsMsg(Vector<String> assetIds, Vector<String> packIds)
	: assetIds(std::move(assetIds))
	, packIds(std::move(packIds))
{
}

void ReloadAssetsMsg::serialize(Serializer& s) const
{
	s << assetIds;
	s << packIds;
}

void ReloadAssetsMsg::deserialize(Deserializer& s)
{
	s >> assetIds;
	s >> packIds;
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

SetClientDataMsg::SetClientDataMsg(GamePlatform platform, String deviceName, ConfigNode params)
	: platform(platform)
	, deviceName(std::move(deviceName))
	, params(std::move(params))
{
}

void SetClientDataMsg::serialize(Serializer& s) const
{
	s << platform;
	s << deviceName;
	s << params;
}

void SetClientDataMsg::deserialize(Deserializer& s)
{
	s >> platform;
	s >> deviceName;
	s >> params;
}

RPCMsg::RPCMsg(uint64_t id, String method, ConfigNode params)
	: id(id)
	, method(std::move(method))
	, params(std::move(params))
{
}

void RPCMsg::serialize(Serializer& s) const
{
	s << id;
	s << method;
	s << params;
}

void RPCMsg::deserialize(Deserializer& s)
{
	s >> id;
	s >> method;
	s >> params;
}


RPCReplyMsg::RPCReplyMsg(uint64_t id, ConfigNode result)
	: id(id)
	, result(std::move(result))
{
}

void RPCReplyMsg::serialize(Serializer& s) const
{
	s << id;
	s << result;
}

void RPCReplyMsg::deserialize(Deserializer& s)
{
	s >> id;
	s >> result;
}

UpdateStringsMsg::UpdateStringsMsg(I18NLanguage language, HashMap<String, String> strings)
	: language(std::move(language))
	, strings(std::move(strings))
{
}

void UpdateStringsMsg::serialize(Serializer& s) const
{
	s << language.getISOCode();
	s << strings;
}

void UpdateStringsMsg::deserialize(Deserializer& s)
{
	String languageCode;
	s >> languageCode;
	language = I18NLanguage(languageCode);

	s >> strings;
}
