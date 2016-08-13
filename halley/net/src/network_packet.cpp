#include "network_packet.h"
#include <halley/support/exception.h>

using namespace Halley;

NetworkPacket::NetworkPacket()
{
}

NetworkPacket::NetworkPacket(const char* src, size_t size)
{
	if (size > 1200) {
		throw Exception("Packet too big for network.");
	}
	data.resize(size);
	memcpy(data.data(), src, size);
}

size_t NetworkPacket::copyTo(char* dstBuffer, size_t maxSize) const
{
	if (maxSize < data.size()) {
		throw Exception("Destination buffer is too small for network packet.");
	}
	memcpy(dstBuffer, data.data(), data.size());
	return data.size();
}
