#pragma once

#include "halley/api/network_api.h"
#include "halley/net/connection/network_service.h"

#if defined(WITH_GDK)
#include <Windows.h>
#include <winhttp.h>
#endif

#if defined(_WIN32)
#include <WinSock2.h>
#include <ws2tcpip.h>
#endif

namespace Halley
{
	struct Endpoint
	{
		union {
			in6_addr v6;
			in_addr v4;
		} addr;
		uint16_t port; // stored in HOST byte order (little endian)
		IPVersion version;
	};

	using Socket = int64_t;

	class SocketIOConnection : public IConnection
	{
	public:
		explicit SocketIOConnection(Socket socket, NetworkProtocol protocol, Endpoint remote, INetworkServiceStatsListener& statsListener);
		~SocketIOConnection() override;

		void close() override;

		void update();

		[[nodiscard]] ConnectionStatus getStatus() const override { return status; }
		[[nodiscard]] bool isSupported(TransmissionType type) const override;

		void send(TransmissionType type, OutboundNetworkPacket packet) override;
		bool receive(InboundNetworkPacket& packet) override;

		[[nodiscard]] bool matchesEndpoint(const Endpoint& remoteEndpoint) const;
		[[nodiscard]] short getConnectionId() const { return connectionId; }
		[[nodiscard]] String getRemoteAddress() const override;

		[[nodiscard]] size_t getMaxUnreliablePacketSize() const override;

		void onConnect(short connId) override;

		void sendUnreliablePacket(gsl::span<const gsl::byte> packet) override;
		void receiveUnreliablePacket(gsl::span<const gsl::byte> packet) const;
		void setUnreliablePacketListener(IPacketListener* listener) override;

	private:
		const Socket sock;
		const NetworkProtocol protocol;
		const Endpoint remote;

		ConnectionStatus status;
		short connectionId;

		std::list<Bytes> sendQueue;
		Bytes receiveQueue;

		IPacketListener* packetListener = nullptr;
		INetworkServiceStatsListener& statsListener;

		mutable std::mutex mutex;

		String disconnectReason;

		void trySend();
		void tryReceive();
		void disconnect(const String& reason);
	};

	class SocketIONetworkService : public NetworkServiceWithStats
	{
	public:
		explicit SocketIONetworkService(int port, NetworkProtocol protocol, IPVersion version);
		~SocketIONetworkService() override;

		void update(Time t) override;

		String startListening(AcceptCallback callback) override;
		void stopListening() override;

		std::shared_ptr<IConnection> connect(const String& address) override;

	private:
		class SocketAcceptor : public Acceptor
		{
		public:
			explicit SocketAcceptor(SocketIONetworkService& service, Socket socket, const Endpoint& endpoint);
			std::shared_ptr<IConnection> doAccept() override;
			void doReject() override;

		private:
			SocketIONetworkService& service;
			Socket sock;
			Endpoint endpoint;
		};

		Socket sock = -1;
		const NetworkProtocol protocol;

		Endpoint localEndpoint = {};
		Endpoint remoteEndpoint = {};

		HashMap<short, std::shared_ptr<SocketIOConnection>> activeConnections;

		AcceptCallback acceptCallback = {};
		bool startedListening = false;

		void tryListen();
		void tryReceiveUnreliable();

		void receivePacket(const Endpoint& endpoint, gsl::span<gsl::byte> data);

        [[nodiscard]] bool hasConnectionWithId(short connId) const override;

		std::shared_ptr<SocketIOConnection> doAcceptConnection(Socket socket, const Endpoint& endpoint);
		void doRejectConnection();
	};

}
