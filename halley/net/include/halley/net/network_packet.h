#pragma once
#include <vector>

namespace Halley
{
	class NetworkPacket
	{
	public:
		NetworkPacket();
		NetworkPacket(const char* data, size_t size);
		size_t copyTo(char* dstBuffer, size_t maxSize) const;

	private:
		std::vector<char> data;
	};
}
