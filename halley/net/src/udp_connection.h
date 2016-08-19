#pragma once

#include "iconnection.h"
#include <boost/asio.hpp>
#include <list>
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
		
		void terminateConnection();

	private:
		UDPSocket& socket;
		UDPEndpoint remote;
		ConnectionStatus status;

		// TODO: replace these with more efficient structures
		std::list<OutboundNetworkPacket> pendingSend;
		std::list<InboundNetworkPacket> pendingReceive;
		std::array<gsl::byte, 2048> sendBuffer;
		std::string error;

		void sendNext();
		void onClose();
	};
}
