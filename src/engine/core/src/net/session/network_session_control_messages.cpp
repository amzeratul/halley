#include "halley/net/session/network_session_control_messages.h"
using namespace Halley;

void ControlMsgJoin::serialize(Serializer& s) const
{
	s << networkVersion;
	s << userName;
}

void ControlMsgJoin::deserialize(Deserializer& s)
{
	s >> networkVersion;
	s >> userName;
}

void ControlMsgSetPeerId::serialize(Serializer& s) const
{
	s << peerId;
}

void ControlMsgSetPeerId::deserialize(Deserializer& s)
{
	s >> peerId;
}

void ControlMsgSetSessionState::serialize(Serializer& s) const
{
	s << state;
}

void ControlMsgSetSessionState::deserialize(Deserializer& s)
{
	s >> state;
}

void ControlMsgSetPeerState::serialize(Serializer& s) const
{
	s << peerId;
	s << state;
}

void ControlMsgSetPeerState::deserialize(Deserializer& s)
{
	s >> peerId;
	s >> state;
}

void ControlMsgSetServerSideData::serialize(Serializer& s) const
{
	s << key;
	s << data;
	s << requestId;
}

void ControlMsgSetServerSideData::deserialize(Deserializer& s)
{
	s >> key;
	s >> data;
	s >> requestId;
}

void ControlMsgSetServerSideDataReply::serialize(Serializer& s) const
{
	s << ok;
	s << requestId;
}

void ControlMsgSetServerSideDataReply::deserialize(Deserializer& s)
{
	s >> ok;
	s >> requestId;
}

void ControlMsgGetServerSideData::serialize(Serializer& s) const
{
	s << key;
	s << requestId;
}

void ControlMsgGetServerSideData::deserialize(Deserializer& s)
{
	s >> key;
	s >> requestId;
}

void ControlMsgGetServerSideDataReply::serialize(Serializer& s) const
{
	s << data;
	s << requestId;
}

void ControlMsgGetServerSideDataReply::deserialize(Deserializer& s)
{
	s >> data;
	s >> requestId;
}
