#pragma once

#include "iconnection.h"
#include "network_packet.h"
#define BOOST_SYSTEM_NO_DEPRECATED
#include <boost/asio.hpp>
#include <deque>
#include <array>
#include <string>
#include <gsl/gsl>

namespace Halley
{
	class NetworkService;
	using UDPEndpoint = boost::asio::ip::udp::endpoint;
	using UDPSocket = boost::asio::ip::udp::socket;

	class UDPConnection : public IConnection
	{
	public:
		UDPConnection(UDPSocket& socket, UDPEndpoint remote);

		void close() override;
		ConnectionStatus getStatus() const override { return status; }
		void send(OutboundNetworkPacket&& packet) override;
		bool receive(InboundNetworkPacket& packet) override;
		
		bool matchesEndpoint(const UDPEndpoint& remoteEndpoint) const;
		void onReceive(gsl::span<const gsl::byte> data);
		void setError(const std::string& cs);
		
		void open(short connectionId);
		void onOpen(short connectionId);
		void terminateConnection();
		short getConnectionId() const { return connectionId; }

	private:
		UDPSocket& socket;
		UDPEndpoint remote;
		ConnectionStatus status;
		short connectionId;

		std::deque<OutboundNetworkPacket> pendingSend;
		std::deque<InboundNetworkPacket> pendingReceive;
		std::array<gsl::byte, 2048> sendBuffer;
		std::string error;

		void sendNext();
	};
}
