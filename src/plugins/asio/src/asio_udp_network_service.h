#pragma once
#include "halley/net/connection/network_service.h"

#ifdef _MSC_VER
#pragma warning(disable: 4834)
#endif
#define BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/asio.hpp>

#include "halley/data_structures/hash_map.h"
namespace asio = boost::asio;

#include "asio_udp_connection.h"

namespace Halley
{
	class AsioUDPNetworkService : public NetworkService
	{
	public:
		AsioUDPNetworkService(int port, IPVersion version = IPVersion::IPv4);
		~AsioUDPNetworkService();

		void update() override;

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

		asio::io_service service;
		UDPEndpoint localEndpoint;
		UDPEndpoint remoteEndpoint;
		asio::ip::udp::socket socket;
		HashMap<short, std::shared_ptr<AsioUDPConnection>> activeConnections;

		std::array<gsl::byte, 2048> receiveBuffer;

		void receiveNext();
		void receivePacket(gsl::span<gsl::byte> data, std::string* error);
		bool isValidConnectionRequest(gsl::span<const gsl::byte> data);
		short getFreeId() const;

		std::shared_ptr<AsioUDPConnection> acceptConnection(UDPEndpoint endPoint);
		void rejectConnection();
	};

}
