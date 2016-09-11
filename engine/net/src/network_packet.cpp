#include "network_packet.h"
#include <halley/support/exception.h>
#include <cassert>

using namespace Halley;

NetworkPacketBase::NetworkPacketBase()
	: dataStart(0)
{}

NetworkPacketBase::NetworkPacketBase(gsl::span<const gsl::byte> src, size_t prePadding)
	: dataStart(prePadding)
{
	if (src.size_bytes() + prePadding > 1500) {
		throw Exception("Packet too big for network.");
	}
	data.resize(src.size_bytes() + prePadding);
	memcpy(data.data() + prePadding, src.data(), src.size_bytes());
}

size_t NetworkPacketBase::copyTo(gsl::span<gsl::byte> dst) const
{
	if (dst.size() < signed(getSize())) {
		throw Exception("Destination buffer is too small for network packet.");
	}
	memcpy(dst.data(), data.data() + dataStart, getSize());
	return getSize();
}

size_t NetworkPacketBase::getSize() const
{
	Expects(dataStart < 1500);
	Expects(data.size() >= dataStart);
	return data.size() - dataStart;
}

gsl::span<const gsl::byte> NetworkPacketBase::getBytes() const
{
	return gsl::span<const gsl::byte>(data).subspan(dataStart, getSize());
}

OutboundNetworkPacket::OutboundNetworkPacket(const OutboundNetworkPacket& other)
	: NetworkPacketBase()
{
	data = other.data;
	dataStart = other.dataStart;
}

OutboundNetworkPacket::OutboundNetworkPacket(OutboundNetworkPacket&& other)
{
	data = other.data;
	dataStart = other.dataStart;
	other.dataStart = 0;
}

OutboundNetworkPacket::OutboundNetworkPacket(gsl::span<const gsl::byte> data)
	: NetworkPacketBase(data, 128)
{}

void OutboundNetworkPacket::addHeader(gsl::span<const gsl::byte> src)
{
	Expects(src.size_bytes() <= signed(dataStart));
	
	dataStart -= src.size_bytes();
	memcpy(data.data() + dataStart, src.data(), src.size_bytes());

	Ensures(dataStart <= 1500);
}

OutboundNetworkPacket& OutboundNetworkPacket::operator=(OutboundNetworkPacket&& other) 
{
	data = other.data;
	dataStart = other.dataStart;
	other.dataStart = 0;
	return *this;
}

InboundNetworkPacket::InboundNetworkPacket()
	: NetworkPacketBase()
{}

InboundNetworkPacket::InboundNetworkPacket(InboundNetworkPacket&& other)
	: NetworkPacketBase()
{
	data = other.data;
	dataStart = other.dataStart;
	other.dataStart = 0;
}

InboundNetworkPacket::InboundNetworkPacket(gsl::span<const gsl::byte> data)
	: NetworkPacketBase(data, 0)
{}

void InboundNetworkPacket::extractHeader(gsl::span<gsl::byte> dst)
{
	Expects(dst.size_bytes() <= signed(data.size()));

	memcpy(dst.data(), data.data() + dataStart, dst.size_bytes());
	dataStart += dst.size_bytes();

	Ensures(dataStart <= 1500);
}

InboundNetworkPacket& InboundNetworkPacket::operator=(InboundNetworkPacket&& other)
{
	data = other.data;
	dataStart = other.dataStart;
	other.dataStart = 0;
	return *this;
}
