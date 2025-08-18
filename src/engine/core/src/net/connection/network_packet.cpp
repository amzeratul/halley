#include "halley/net/connection/network_packet.h"
#include <halley/support/exception.h>
#include <cassert>

#include "halley/support/logger.h"

using namespace Halley;

NetworkPacketBase::NetworkPacketBase()
	: dataStart(0)
{}

NetworkPacketBase::NetworkPacketBase(gsl::span<const std::byte> src, size_t prePadding)
	: dataStart(prePadding)
{
	data.resize(src.size_bytes() + prePadding);
	memcpy(data.data() + prePadding, src.data(), src.size_bytes());
}

size_t NetworkPacketBase::copyTo(gsl::span<std::byte> dst) const
{
	if (getSize() == 0) {
		return 0;
	}

	if (dst.size() < signed(getSize())) {
		throw Exception("Destination buffer is too small for network packet.", HalleyExceptions::Network);
	}
	memcpy(dst.data(), data.data() + dataStart, getSize());
	return getSize();
}

size_t NetworkPacketBase::getSize() const
{
	Expects(data.size() >= dataStart);
	return data.size() - dataStart;
}

gsl::span<const std::byte> NetworkPacketBase::getBytes() const
{
	return gsl::span<const std::byte>(data).subspan(dataStart, getSize());
}

OutboundNetworkPacket::OutboundNetworkPacket(const OutboundNetworkPacket& other)
	: NetworkPacketBase()
{
	data = other.data;
	dataStart = other.dataStart;
}

OutboundNetworkPacket::OutboundNetworkPacket(OutboundNetworkPacket&& other) noexcept
{
	data = other.data;
	dataStart = other.dataStart;
	other.dataStart = 0;
}

OutboundNetworkPacket::OutboundNetworkPacket(gsl::span<const std::byte> data)
	: NetworkPacketBase(data, 128)
{
}

OutboundNetworkPacket::OutboundNetworkPacket(const Bytes& data)
	: NetworkPacketBase(gsl::as_bytes(gsl::span<const Byte>(data)), 128)
{
}

void OutboundNetworkPacket::addHeader(gsl::span<const std::byte> src)
{
	Expects(src.size_bytes() <= dataStart);
	
	dataStart -= src.size_bytes();
	memcpy(data.data() + dataStart, src.data(), src.size_bytes());
}

OutboundNetworkPacket& OutboundNetworkPacket::operator=(OutboundNetworkPacket&& other) noexcept
{
	data = std::move(other.data);
	dataStart = other.dataStart;
	other.dataStart = 0;
	return *this;
}

InboundNetworkPacket::InboundNetworkPacket()
	: NetworkPacketBase()
{}

InboundNetworkPacket::InboundNetworkPacket(InboundNetworkPacket&& other) noexcept
	: NetworkPacketBase()
{
	data = std::move(other.data);
	dataStart = other.dataStart;
	other.dataStart = 0;
}

InboundNetworkPacket::InboundNetworkPacket(gsl::span<const std::byte> data)
	: NetworkPacketBase(data, 0)
{}

void InboundNetworkPacket::extractHeader(gsl::span<std::byte> dst)
{
	Expects(dst.size_bytes() <= signed(data.size()));

	memcpy(dst.data(), data.data() + dataStart, dst.size_bytes());
	dataStart += dst.size_bytes();
}

InboundNetworkPacket& InboundNetworkPacket::operator=(InboundNetworkPacket&& other) noexcept
{
	data = other.data;
	dataStart = other.dataStart;
	other.dataStart = 0;
	return *this;
}
