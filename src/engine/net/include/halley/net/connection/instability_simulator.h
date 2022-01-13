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
			DelayedPacket(std::chrono::system_clock::time_point when, OutboundNetworkPacket packet);
			bool operator<(const DelayedPacket& other) const;
			bool isReady() const;

			std::chrono::system_clock::time_point when;
			OutboundNetworkPacket packet;
		};

	public:
		explicit InstabilitySimulator(std::shared_ptr<IConnection> parent, float avgLag, float lagVariance, float packetLoss, float duplication);
		void close() override;
		ConnectionStatus getStatus() const override;
		void send(OutboundNetworkPacket packet) override;
		bool receive(InboundNetworkPacket& packet) override;

	private:
		std::shared_ptr<IConnection> parent;
		float avgLag;
		float lagVariance;
		float packetLoss;
		float duplication;

		std::priority_queue<DelayedPacket> packets;
		void sendPendingPackets();
	};
}
