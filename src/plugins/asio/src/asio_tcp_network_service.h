#pragma once
#include "halley/net/connection/network_service.h"
#include "asio_tcp_connection.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class AsioTCPNetworkService : public NetworkServiceWithStats
	{
	public:
		AsioTCPNetworkService(int port, IPVersion version = IPVersion::IPv4);

		void update(Time t) override;
		String startListening(AcceptCallback callback) override;
		void stopListening() override;
		std::shared_ptr<IConnection> connect(const String& address) override;

	private:
		class TCPAcceptor : public Acceptor {
		public:
			TCPAcceptor(AsioTCPNetworkService& service);
			std::shared_ptr<IConnection> doAccept() override;
			void doReject() override;

		private:
			AsioTCPNetworkService& service;
		};
		
		asio::io_service service;
		asio::io_service::work work;
		TCPEndpoint localEndpoint;
		asio::ip::tcp::acceptor acceptor;

		AcceptCallback acceptCallback;
		std::optional<TCPSocket> acceptingSocket;
		
		Vector<std::shared_ptr<AsioTCPConnection>> activeConnections;

		std::shared_ptr<AsioTCPConnection> acceptConnection();
		void rejectConnection();
		void doStartListening();
	};
}
