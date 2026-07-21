#pragma once

#include "iconnection.h"
#include "network_packet.h"
#include <memory>
#include <queue>
#include <chrono>

namespace Halley
{
	class InstabilitySimulator : public IConnection
	{
		class DelayedPacket
		{
		public:
			DelayedPacket(std::chrono::system_clock::time_point when, TransmissionType type, OutboundNetworkPacket packet, uint64_t id);
			bool operator<(const DelayedPacket& other) const;
			[[nodiscard]] bool isReady() const;

			std::chrono::system_clock::time_point when;
			TransmissionType type;
			OutboundNetworkPacket packet;
			uint64_t id;
		};

	public:
		explicit InstabilitySimulator(std::shared_ptr<IConnection> parent);

		void setLag(float average, float variance);
		void setPacketLoss(float chance);
		void setDuplication(float chance);

		void close() override;
		[[nodiscard]] ConnectionStatus getStatus() const override;

		[[nodiscard]] bool isSupported(TransmissionType type) const override;
		void send(TransmissionType type, OutboundNetworkPacket packet) override;
		bool receive(InboundNetworkPacket& packet) override;

		[[nodiscard]] size_t getMaxUnreliablePacketSize() const override;
		void onConnect(short connId) override;
		void sendUnreliablePacket(gsl::span<const std::byte> packet, uint64_t id) override;
		void setUnreliablePacketListener(IPacketListener* listener) override;

	private:
		std::shared_ptr<IConnection> parent;
		bool active = false;

		float avgLag;
		float lagVariance;
		float packetLoss;
		float duplication;

		std::priority_queue<DelayedPacket> packets;
		void sendPendingPackets();

		void checkActive();
	};
}
