#pragma once
#include <vector>
#include <gsl/gsl>

namespace Halley
{
	class NetworkPacketBase
	{
	public:
		size_t copyTo(gsl::span<gsl::byte> dst) const;
		size_t getSize() const;
		gsl::span<const gsl::byte> getBytes() const;

		NetworkPacketBase(NetworkPacketBase&& other) = delete;
		NetworkPacketBase& operator=(NetworkPacketBase&& other) = delete;

	protected:
		NetworkPacketBase();
		NetworkPacketBase(gsl::span<const gsl::byte> data, size_t prePadding);

		size_t dataStart;
		std::vector<gsl::byte> data;
	};

	class OutboundNetworkPacket : public NetworkPacketBase
	{
	public:
		OutboundNetworkPacket(const OutboundNetworkPacket& other);
		explicit OutboundNetworkPacket(OutboundNetworkPacket&& other) noexcept;
		explicit OutboundNetworkPacket(gsl::span<const gsl::byte> data);
		void addHeader(gsl::span<const gsl::byte> src);

		OutboundNetworkPacket& operator=(OutboundNetworkPacket&& other) noexcept;
	};

	class InboundNetworkPacket : public NetworkPacketBase
	{
	public:
		InboundNetworkPacket();
		explicit InboundNetworkPacket(InboundNetworkPacket&& other);
		explicit InboundNetworkPacket(gsl::span<const gsl::byte> data);
		void extractHeader(gsl::span<gsl::byte> dst);

		InboundNetworkPacket& operator=(InboundNetworkPacket&& other);
	};
}
