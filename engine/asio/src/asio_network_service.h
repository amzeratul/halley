#pragma once
#include "halley/net/connection/network_service.h"

#define BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/asio.hpp>

#include "asio_udp_connection.h"
#include <unordered_map>
namespace asio = boost::asio;

namespace Halley
{
	class AsioNetworkService : public NetworkService
	{
	public:
		AsioNetworkService(int port, IPVersion version = IPVersion::IPv4);
		~AsioNetworkService();

		void update() override;

		void setAcceptingConnections(bool accepting) override;
		std::shared_ptr<IConnection> tryAcceptConnection() override;
		std::shared_ptr<IConnection> connect(String address, int port) override;

	private:
		bool acceptingConnections = false;
		bool startedListening = false;

		asio::io_service service;
		UDPEndpoint localEndpoint;
		UDPEndpoint remoteEndpoint;
		asio::ip::udp::socket socket;
		std::list<UDPEndpoint> pendingIncomingConnections;
		std::unordered_map<short, std::shared_ptr<AsioUDPConnection>> activeConnections;

		std::array<gsl::byte, 2048> receiveBuffer;

		void startListening();
		void receiveNext();
		void receivePacket(gsl::span<gsl::byte> data, std::string* error);
		bool isValidConnectionRequest(gsl::span<const gsl::byte> data);
		short getFreeId() const;
	};

}
