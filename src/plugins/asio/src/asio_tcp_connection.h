#pragma once

#define BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/asio.hpp>
#include "halley/net/connection/iconnection.h"
#include "halley/utils/utils.h"
namespace asio = boost::asio;

namespace Halley
{
	class String;
	using TCPEndpoint = asio::ip::tcp::endpoint;
	using TCPSocket = asio::ip::tcp::socket;

	class AsioTCPConnection : public IConnection
	{
	public:
		AsioTCPConnection(asio::io_service& service, String host, int port);
		AsioTCPConnection(asio::io_service& service, TCPSocket socket);
		~AsioTCPConnection();

		void update();

		void close() override;
		ConnectionStatus getStatus() const override;
		void send(OutboundNetworkPacket&& packet) override;
		bool receive(InboundNetworkPacket& packet) override;

	private:
		asio::io_service& service;
		std::unique_ptr<asio::ip::tcp::resolver> resolver;
		TCPSocket socket;
		ConnectionStatus status;

		std::list<Bytes> sendQueue;
		Bytes receiveBuffer;
		Bytes receiveQueue;

		void sendMore();
		void sendBuffer(Bytes& buffer);

		void tryReceive();
	};
}
