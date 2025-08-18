#pragma once
#include "halley/net/connection/network_service.h"

#ifdef _MSC_VER
#pragma warning(disable: 4834)
#endif

#include <asio.hpp>

#include "halley/data_structures/hash_map.h"
#include "asio_udp_connection.h"

namespace Halley
{
	class AsioUDPNetworkService : public NetworkServiceWithStats
	{
	public:
		explicit AsioUDPNetworkService(int port, IPVersion version = IPVersion::IPv4);
		~AsioUDPNetworkService() override;

		void update(Time t) override;

		String startListening(AcceptCallback callback) override;
		void stopListening() override;
		std::shared_ptr<IConnection> connect(const String& address) override;

	private:
		class UDPAcceptor : public Acceptor {
		public:
			UDPAcceptor(AsioUDPNetworkService& service, UDPEndpoint endPoint);
			std::shared_ptr<IConnection> doAccept() override;
			void doReject() override;

		private:
			AsioUDPNetworkService& service;
			UDPEndpoint endPoint;
		};
		
		AcceptCallback acceptCallback;
		bool startedListening = false;

		asio::io_context service;
		UDPEndpoint localEndpoint;
		UDPEndpoint remoteEndpoint;
		asio::ip::udp::socket socket;
		HashMap<short, std::shared_ptr<AsioUDPConnection>> activeConnections;

		void receivePacket(UDPEndpoint& endpoint, gsl::span<std::byte> data, std::string* error);

        [[nodiscard]] bool hasConnectionWithId(short connId) const override;

		std::shared_ptr<AsioUDPConnection> doAcceptConnection(UDPEndpoint endPoint);
		void doRejectConnection();
	};

}
