#include "network_packet.h"
#include <cstring>

using namespace Halley;

size_t NetworkPacket::copyTo(char* dstBuffer, size_t maxSize)
{
	const char* lulz = "Hello world!";
	strcpy(dstBuffer, lulz);
	return strlen(lulz) + 1;
}
