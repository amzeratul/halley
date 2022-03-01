#include <halley/support/exception.h>
#include "connection/message_queue.h"
#include "connection/ack_unreliable_connection.h"
#include "halley/text/string_converter.h"

using namespace Halley;


MessageQueue::~MessageQueue()
{
}

void MessageQueue::setChannel(int channel, ChannelSettings settings)
{
}

