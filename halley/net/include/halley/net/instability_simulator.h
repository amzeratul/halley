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
			DelayedPacket(std::chrono::system_clock::time_point when, NetworkPacket&& packet);
			bool operator<(const DelayedPacket& other) const;
			bool isReady() const;

			std::chrono::system_clock::time_point when;
			NetworkPacket packet;
		};

	public:
		explicit InstabilitySimulator(std::shared_ptr<IConnection> parent, float avgLag, float lagVariance, float packetLoss);
		void close() override;
		ConnectionStatus getStatus() const override;
		void send(NetworkPacket&& packet) override;
		bool receive(NetworkPacket& packet) override;

	private:
		std::shared_ptr<IConnection> parent;
		float avgLag;
		float lagVariance;
		float packetLoss;

		std::priority_queue<DelayedPacket> packets;
		void sendPendingPackets();
	};
}
