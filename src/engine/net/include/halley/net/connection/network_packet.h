#pragma once
#include <vector>
#include <gsl/gsl>
#include "halley/utils/utils.h"

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
		OutboundNetworkPacket(OutboundNetworkPacket&& other) noexcept;
		explicit OutboundNetworkPacket(gsl::span<const gsl::byte> data);
		explicit OutboundNetworkPacket(const Bytes& data);
		
		void addHeader(gsl::span<const gsl::byte> src);

		template <typename T>
		void addHeader(const T& h)
		{
			addHeader(gsl::as_bytes(gsl::span<const T>(&h, 1)));
		}

		OutboundNetworkPacket& operator=(OutboundNetworkPacket&& other) noexcept;
	};

	class InboundNetworkPacket : public NetworkPacketBase
	{
	public:
		InboundNetworkPacket();
		InboundNetworkPacket(InboundNetworkPacket&& other) noexcept;
		explicit InboundNetworkPacket(gsl::span<const gsl::byte> data);
		void extractHeader(gsl::span<gsl::byte> dst);

		template <typename T>
		void extractHeader(T& h)
		{
			extractHeader(gsl::as_writable_bytes(gsl::span<T>(&h, 1)));
		}

		InboundNetworkPacket& operator=(InboundNetworkPacket&& other) noexcept;
	};
}
