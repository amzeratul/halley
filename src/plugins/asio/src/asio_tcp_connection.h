#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4834)
#endif
#include <asio.hpp>
#include "halley/net/connection/iconnection.h"
#include "halley/utils/utils.h"
#include <mutex>

namespace Halley
{
	class String;
    class INetworkServiceStatsListener;
	using TCPEndpoint = asio::ip::tcp::endpoint;
	using TCPSocket = asio::ip::tcp::socket;

	class AsioTCPConnection : public IConnection
	{
	public:
		AsioTCPConnection(asio::io_context& service, String host, int port, INetworkServiceStatsListener& statsListener);
		AsioTCPConnection(asio::io_context& service, TCPSocket socket, INetworkServiceStatsListener& statsListener);
		~AsioTCPConnection();

		void update();
		bool needsPolling() const;

		void close() override;
		ConnectionStatus getStatus() const override;
		bool isSupported(TransmissionType type) const override;
		void send(TransmissionType type, OutboundNetworkPacket packet) override;
		bool receive(InboundNetworkPacket& packet) override;

		String getRemoteAddress() const override;

	private:
		asio::io_context& service;
		std::unique_ptr<asio::ip::tcp::resolver> resolver;
		TCPSocket socket;
        INetworkServiceStatsListener& statsListener;
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
