#pragma once

#include "halley/api/network_api.h"
#include "halley/net/connection/network_service.h"

#ifdef _WIN32
#include <ws2tcpip.h>
#endif

namespace Halley
{
	struct Endpoint
	{
		union
		{
			in6_addr ipv6;
			struct
			{
				uint8_t zeros[10];
				uint16_t ones = 0xffff;
				in_addr ip;
			} ipv4;
		};
		uint16_t port;
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

		void terminateConnection();
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

		void trySend();
		void tryReceive();
	};

	class SocketIONetworkService : public NetworkServiceWithStats, public NetworkService::Acceptor
	{
	public:
		explicit SocketIONetworkService(int port, NetworkProtocol protocol, IPVersion version);
		~SocketIONetworkService() override;

		void update(Time t) override;

		String startListening(AcceptCallback callback) override;
		void stopListening() override;

		std::shared_ptr<IConnection> connect(const String& address) override;

	private:
		Socket sock = -1;
		const NetworkProtocol protocol;

		Endpoint localEndpoint = {};
		Endpoint remoteEndpoint = {};

		HashMap<short, std::shared_ptr<SocketIOConnection>> activeConnections;

		AcceptCallback acceptCallback = {};
		Socket acceptSock = -1;
		Endpoint acceptEndpoint = {};
		bool startedListening = false;

		void tryListen();
		void tryReceiveUnreliable();

		void receivePacket(const Endpoint& endpoint, gsl::span<gsl::byte> data);

        [[nodiscard]] bool hasConnectionWithId(short connId) const override;

		std::shared_ptr<IConnection> doAccept() override;
		void doReject() override;

		static void setEndpointFromAddrInfo(Endpoint& endpoint, uint16_t port, const addrinfo* addr);
		static void setEndpointFromSockAddr(Endpoint& endpoint, const sockaddr* addr, int addrLen);
	};

	extern void setSockAddrFromEndpoint(const Endpoint& endpoint, sockaddr* addr, int* addrLen);

}
