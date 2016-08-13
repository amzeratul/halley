#pragma once

namespace Halley
{
	class NetworkPacket
	{
	public:
		size_t copyTo(char* dstBuffer, size_t maxSize);
	};
}
