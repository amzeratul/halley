#pragma once

#include "iconnection.h"
#include <boost/asio.hpp>

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
		void send(NetworkPacket&& packet) override;
		bool receive(NetworkPacket& packet) override;

	private:
		UDPSocket& socket;
		UDPEndpoint remote;

		std::list<NetworkPacket> pendingSend;
		std::array<char, 2048> sendBuffer;

		void sendNext();
	};
}
