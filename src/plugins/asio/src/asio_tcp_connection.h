#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4834)
#endif
#define BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/asio.hpp>
#include "halley/net/connection/iconnection.h"
#include "halley/utils/utils.h"
#include <mutex>
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
		bool needsPolling() const;

		void close() override;
		ConnectionStatus getStatus() const override;
		bool isSupported(TransmissionType type) const override;
		void send(TransmissionType type, OutboundNetworkPacket packet) override;
		bool receive(InboundNetworkPacket& packet) override;

	private:
		asio::io_service& service;
		std::unique_ptr<asio::ip::tcp::resolver> resolver;
		TCPSocket socket;
		ConnectionStatus status;

		std::list<Bytes> sendQueue;
		std::list<Bytes> sendingQueue;
		Bytes receiveQueue;
		Bytes receiveBuffer;
		bool reading = false;
		bool needsPoll = false;

		mutable std::mutex mutex;

		void trySend();
		void tryReceive();
	};
}
