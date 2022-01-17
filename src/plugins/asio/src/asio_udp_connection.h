#pragma once

#include "halley/net/connection/iconnection.h"
#include "halley/net/connection/network_packet.h"

#ifdef _MSC_VER
#pragma warning(disable: 4834)
#endif
#define BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_ERROR_CODE_HEADER_ONLY
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

	class AsioUDPConnection : public IConnection
	{
	public:
		AsioUDPConnection(UDPSocket& socket, UDPEndpoint remote);

		void close() override;
		ConnectionStatus getStatus() const override { return status; }
		bool isSupported(TransmissionType type) const override;
		void send(TransmissionType type, OutboundNetworkPacket packet) override;
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
