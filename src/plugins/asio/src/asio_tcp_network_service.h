#pragma once
#include "halley/net/connection/network_service.h"
#include "asio_tcp_connection.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class AsioTCPNetworkService : public NetworkService
	{
	public:
		AsioTCPNetworkService(int port, IPVersion version = IPVersion::IPv4);

		void update() override;
		void setAcceptingConnections(bool accepting) override;
		std::shared_ptr<IConnection> tryAcceptConnection() override;
		std::shared_ptr<IConnection> connect(String address, int port) override;

	private:
		asio::io_service service;
		TCPEndpoint localEndpoint;
		asio::ip::tcp::acceptor acceptor;

		bool acceptingConnection = false;
		Maybe<TCPSocket> acceptingSocket;

		std::vector<std::shared_ptr<IConnection>> pendingConnections;
		std::vector<std::shared_ptr<IConnection>> activeConnections;

		void onConnectionAccepted();
	};
}
