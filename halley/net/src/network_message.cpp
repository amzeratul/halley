#include "network_message.h"
#include <imessage_stream.h>

using namespace Halley;

void NetworkMessage::setStream(gsl::not_null<IMessageStream*> s)
{
	stream = s;
}

bool NetworkMessage::isReliable() const
{
	Expects(stream != nullptr);
	return stream->isReliable();
}

void NetworkMessage::onAck()
{
	// TODO
}
