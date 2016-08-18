#include "network_packet.h"
#include <halley/support/exception.h>
#include <cassert>

using namespace Halley;

NetworkPacket::NetworkPacket()
{
}

NetworkPacket::NetworkPacket(gsl::span<const gsl::byte> src)
{
	if (src.size_bytes() > 1500) {
		throw Exception("Packet too big for network.");
	}
	data.resize(src.size_bytes());
	memcpy(data.data(), src.data(), src.size_bytes());
}

size_t NetworkPacket::copyTo(gsl::span<gsl::byte> dst) const
{
	if (dst.size() < signed(data.size())) {
		throw Exception("Destination buffer is too small for network packet.");
	}
	memcpy(dst.data(), data.data(), data.size());
	return data.size();
}

size_t NetworkPacket::getSize() const
{
	return data.size();
}

const char* NetworkPacket::getData() const
{
	return data.data();
}

void NetworkPacket::addHeader(const gsl::span<gsl::byte> src)
{
	Expects(src.size_bytes() < 64);
	std::vector<char> newData(data.size() + src.size_bytes());
	memcpy(newData.data(), src.data(), src.size_bytes());
	memcpy(newData.data() + src.size_bytes(), data.data(), data.size());
	data = std::move(newData);
}

void NetworkPacket::extractHeader(gsl::span<gsl::byte> dst)
{
	Expects(dst.size_bytes() <= signed(data.size()));
	memcpy(dst.data(), data.data(), dst.size_bytes());
	std::vector<char> newData(data.size() - dst.size_bytes());
	memcpy(newData.data(), data.data() + dst.size_bytes(), data.size() - dst.size_bytes());
	data = std::move(newData);
}
