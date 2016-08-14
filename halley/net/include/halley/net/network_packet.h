#pragma once
#include <vector>

namespace Halley
{
	class NetworkPacket
	{
	public:
		NetworkPacket();
		NetworkPacket(const char* data, size_t size);

		NetworkPacket(const NetworkPacket& packet) = default;
		NetworkPacket(NetworkPacket&& packet) = default;
		NetworkPacket& operator=(const NetworkPacket& packet) = default;
		NetworkPacket& operator=(NetworkPacket&& packet) = default;

		size_t copyTo(char* dstBuffer, size_t maxSize) const;

		size_t getSize() const;
		const char* getData() const;

	private:
		std::vector<char> data;
	};
}
