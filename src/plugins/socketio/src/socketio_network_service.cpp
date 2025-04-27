#include "socketio_network_service.h"

#include "halley/net/connection/network_packet.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace Halley;

SocketIOConnection::SocketIOConnection(Socket socket, NetworkProtocol protocol, Endpoint remote, INetworkServiceStatsListener& statsListener)
	: sock(socket)
	, protocol(protocol)
	, remote(remote)
	, status(protocol == NetworkProtocol::TCP ? ConnectionStatus::Connected : ConnectionStatus::Connecting)
	, connectionId(0)
	, statsListener(statsListener)
{

}

SocketIOConnection::~SocketIOConnection()
{
	SocketIOConnection::close();
}

void SocketIOConnection::close()
{
	if (status != ConnectionStatus::Closed) {
		status = ConnectionStatus::Closed;
	}
}

void SocketIOConnection::update()
{
	if (status == ConnectionStatus::Closing) {
		close();
	}

	if (status == ConnectionStatus::Connected) {
		std::unique_lock lock(mutex);
		tryReceive();
		trySend();
	}
}

bool SocketIOConnection::isSupported(TransmissionType type) const
{
	switch (protocol) {
		case NetworkProtocol::TCP:
			return type == TransmissionType::Reliable;
		case NetworkProtocol::UDP:
			return type == TransmissionType::Unreliable;
	}

	return false;
}

void SocketIOConnection::send(TransmissionType type, OutboundNetworkPacket packet)
{
	Ensures(isSupported(type));

	statsListener.onSendData(packet.getSize(), 1);

	packet.addHeader(static_cast<uint32_t>(packet.getSize()));

	const auto src = packet.getBytes();
	auto dst = Bytes(src.size_bytes());

	memcpy(dst.data(), src.data(), src.size());

	std::unique_lock lock(mutex);
	if (status == ConnectionStatus::Connected || status == ConnectionStatus::Connecting) {
		sendQueue.emplace_back(std::move(dst));
		trySend();
	}
}

void SocketIOConnection::trySend()
{
	// NB: make sure any caller has acquired mutex lock
	while (!sendQueue.empty() && status == ConnectionStatus::Connected) {
		// Send some data.
		auto& front = sendQueue.front();
		const int bytesSent = ::send(sock, reinterpret_cast<const char *>(front.data()), static_cast<int>(front.size_bytes()), 0);

		if (bytesSent == SOCKET_ERROR) {
			Logger::logError("Error sending data on TCP socket");
			close();
			break;
		}

		if (bytesSent == front.size_bytes()) {
			sendQueue.pop_front();
		} else {
			front.erase(front.begin(), front.begin() + bytesSent);
		}
	}
}

bool SocketIOConnection::receive(InboundNetworkPacket& packet)
{
	std::unique_lock lock(mutex);

	if (receiveQueue.size() >= sizeof(uint32_t) && status == ConnectionStatus::Connected) {
		const uint32_t size = *reinterpret_cast<uint32_t*>(receiveQueue.data());
		if (size > 128 * 1024 * 1024) {
			Logger::logError("Invalid packet size.");
			close();
			return false;
		}

		const auto packetSize = sizeof(uint32_t) + size;
		if (receiveQueue.size() >= packetSize) {
			packet = InboundNetworkPacket(gsl::as_bytes(gsl::span(receiveQueue.data(), packetSize)));
			uint32_t size2;
			packet.extractHeader(size2);
			receiveQueue.erase(receiveQueue.begin(), receiveQueue.begin() + packetSize);
			statsListener.onReceiveData(packet.getSize(), 1);
			return true;
		}
	}

	return false;
}

void SocketIOConnection::tryReceive()
{
	if (status == ConnectionStatus::Connected) {
		fd_set rd = {1};
		rd.fd_array[0] = sock;
		constexpr timeval timeout = {0, 0};

		const int err = ::select(0, &rd, nullptr, nullptr, &timeout);

		if (err == 0) {
			return; // No pending data.
		}

		if (err == SOCKET_ERROR) {
			Logger::logError("Error receiving data on TCP socket");
			close();
			return;
		} else if (err != 1) {
			Logger::logWarning("Unexpected TCP socket read status: " + toString(err));
		}
	}

	while (status == ConnectionStatus::Connected) {
		std::array<char, 4096> buffer;
		const int bytesRecv = ::recv(sock, buffer.data(), (int) buffer.size(), 0);

		if (bytesRecv == 0) {
			// "If the connection has been gracefully closed, the return value is zero."
			// TODO:
			break;
		}

		if (bytesRecv == SOCKET_ERROR) {
			const int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) {
				break;
			} else if (err == WSAEMSGSIZE) {
				throw Exception("Not implemented", HalleyExceptions::Network);
			} else {
				Logger::logError("Error receiving data on TCP socket");
				close();
				break;
			}
		}

		size_t curQueueSize = receiveQueue.size();
		receiveQueue.resize(curQueueSize + bytesRecv);
		memcpy(receiveQueue.data() + curQueueSize, buffer.data(), bytesRecv);

		if (bytesRecv < buffer.size()) {
			break; // No more data.
		}
	}
}

bool SocketIOConnection::matchesEndpoint(const Endpoint& remoteEndpoint) const
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

void SocketIOConnection::setError(const std::string& cs)
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

void SocketIOConnection::terminateConnection()
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

String SocketIOConnection::getRemoteAddress() const
{
	// TODO: IPv6
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = remote.port;
	addr.sin_addr = remote.ipv4.ip;

	char node[NI_MAXHOST] = {};
	char serv[NI_MAXSERV] = {};

	int err = getnameinfo(reinterpret_cast<const sockaddr*>(&addr), sizeof(addr), node, sizeof(node), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);

	if (err == 0) {
		return String(node) + ":" + String(serv);
	}

	return IConnection::getRemoteAddress();
}

size_t SocketIOConnection::getMaxUnreliablePacketSize() const
{
	return 1400;
}

void SocketIOConnection::onConnect(short connId)
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

void SocketIOConnection::sendUnreliablePacket(gsl::span<const gsl::byte> packet)
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

void SocketIOConnection::setUnreliablePacketListener(IPacketListener* listener)
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

void SocketIOConnection::receiveAll(
	Socket &socket, HashMap<short, std::shared_ptr<SocketIOConnection>>& connections,
	const std::function<void(Endpoint& remote, gsl::span<gsl::byte> packet)>& unknownConnCallback)
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

SocketIONetworkService::SocketIONetworkService(int port, NetworkProtocol protocol, IPVersion version)
	: protocol(protocol)
{
	Expects(port == 0 || port > 1024);
	Expects(port < 65536);

	localEndpoint.port = port;

	Expects(version == IPVersion::IPv4); // No IPv6 support yet.
}

SocketIONetworkService::~SocketIONetworkService()
{
	if (sock >= 0) {
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
	}
}

void SocketIONetworkService::update(Time t)
{
	NetworkServiceWithStats::update(t);

	Vector<short> toErase;
	for (const auto& [id, conn] : activeConnections) {
		conn->update();
		if (conn->getStatus() == ConnectionStatus::Closed) {
			toErase.push_back(id);
		}
	}

	for (auto connId : toErase) {
		activeConnections.erase(connId);
	}

	tryListen();
}

String SocketIONetworkService::startListening(AcceptCallback callback)
{
	Ensures(!startedListening);

	acceptCallback = std::move(callback);

	if (protocol == NetworkProtocol::TCP) {
		// Create local, listening socket.
#ifdef _WIN32
		const SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (s == INVALID_SOCKET) {
			throw Exception("Failed to create local socket", HalleyExceptions::Network);
		}

		sock = s;
#else
		throw Exception("Not implemented", HalleyExceptions::Network);
#endif

		// Bind to local port.
		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(localEndpoint.port);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

		if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0) {
			throw Exception("Failed to bind local socket", HalleyExceptions::Network);
		}

		// Start to listen.
		if (listen(sock, 1) == SOCKET_ERROR) {
			throw Exception("Failed to listen on local socket", HalleyExceptions::Network);
		}
	} else {
		throw Exception("Not implemented", HalleyExceptions::Network);
	}

	startedListening = true;

	return "";
}

void SocketIONetworkService::stopListening()
{
	Ensures(startedListening);

	acceptCallback = {};
	startedListening = false;

	if (protocol == NetworkProtocol::TCP) {
		if (sock >= 0) {
			closesocket(sock);
			sock = -1;
		}
	}
}

void SocketIONetworkService::tryListen()
{
	if (protocol != NetworkProtocol::TCP || !startedListening) {
		return;
	}

	// Check if there's something waiting on the listen socket.
	fd_set rd = {1};
	rd.fd_array[0] = sock;
	constexpr timeval timeout = {0, 0};

	const int err = ::select(0, &rd, nullptr, nullptr, &timeout);

	if (err == 0) {
		return; // No pending data.
	}

	if (err == SOCKET_ERROR) {
		Logger::logError("Error receiving data on TCP listen socket");
		stopListening();
		return;
	} else if (err != 1) {
		Logger::logWarning("Unexpected TCP socket read status on listening socket: " + toString(err));
	}

	// Accept incoming requests.
	sockaddr remoteAddr = {};
	int remoteAddrLen = sizeof(remoteAddr);

#ifdef _WIN32
	SOCKET a = ::accept(sock, &remoteAddr, &remoteAddrLen);

	if (a == INVALID_SOCKET) {
		throw Exception("Failed to accept incoming request", HalleyExceptions::Network);
	}

	acceptSock = a;
#else
	throw Exception("Not implemented", HalleyExceptions::Network);
#endif

	setEndpointFromSockAddr(remoteEndpoint, &remoteAddr, remoteAddrLen);

	// Obtain local address of the accept socket
	sockaddr localAddr = {};
	int localAddrLen = sizeof(localAddr);
	if (getsockname(acceptSock, &localAddr, &localAddrLen) == 0) {
		setEndpointFromSockAddr(acceptEndpoint, &localAddr, localAddrLen);
	} else {
		Logger::logError("Failed to get local socket name");
	}

	if (acceptCallback) {
		acceptCallback(*this);
		ensureChoiceMade();
	}
}

std::shared_ptr<IConnection> SocketIONetworkService::connect(const String& address)
{
	const auto splitAddr = address.split(':');
	const auto& ip = splitAddr.at(0);
	const auto& portAsString = splitAddr.at(1);
	const uint16_t port = portAsString.toInteger();

	// Figure out socket parameters for connecting to remote address.
	addrinfo hint = {};
	hint.ai_flags = 0; // AI_PASSIVE?
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = protocol == NetworkProtocol::TCP ? SOCK_STREAM : SOCK_DGRAM;

	addrinfo* result;
	if (getaddrinfo(ip.c_str(), portAsString.c_str(), &hint, &result) != 0) {
		Logger::logError("Failed to create local socket to connect to " + address);
		return {};
	}

	// Create the local socket.
#ifdef _WIN32
	const SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (s == INVALID_SOCKET) {
		throw Exception("Failed to create local socket", HalleyExceptions::Network);
	}

	sock = s;
#else
	throw Exception("Not implemented", HalleyExceptions::Network);
#endif

	// TCP_NODELAY
	if (protocol == NetworkProtocol::TCP) {
		uint32_t noDelay = 1;
#ifdef _WIN32
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char *>(&noDelay), sizeof(noDelay));
#else
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));
#endif
	}

	// non-blocking
	if (protocol != NetworkProtocol::TCP) {
#ifdef _WIN32
		DWORD noBlock = 1;
		if (ioctlsocket(sock, FIONBIO, &noBlock)) {
			throw Exception("Failed to set socket to non-blocking", HalleyExceptions::Network);
		}
#else
		if (fcntl(sock, F_SETFL, O_NONBLOCK, 1) == -1) {
			throw Exception("Failed to set socket to non-blocking", HalleyExceptions::Network);
		}
#endif
	}

	// Store the remote address.
	setEndpointFromAddrInfo(remoteEndpoint, port, result);

	// Connect.
	if (::connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
		Logger::logError("Failed to connect socket to " + address);
		freeaddrinfo(result);
		return {};
	}

	// Need to free up this.
	freeaddrinfo(result);

	// Obtain the local address
	sockaddr localAddr = {};
	int localAddrLen = sizeof(localAddr);
	if (getsockname(sock, &localAddr, &localAddrLen) == 0) {
		Expects(localEndpoint.port == 0);
		setEndpointFromSockAddr(localEndpoint, &localAddr, localAddrLen);
	} else {
		Logger::logError("Failed to get local socket name");
	}

	auto conn = std::make_shared<SocketIOConnection>(sock, protocol, remoteEndpoint, *this);
	activeConnections[localEndpoint.port] = conn;

	return conn;
}

void SocketIONetworkService::receivePacket(Endpoint& endpoint, gsl::span<gsl::byte> data, std::string* error)
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

bool SocketIONetworkService::hasConnectionWithId(short connId) const
{
    throw Exception("Not implemented", HalleyExceptions::Network);
}

std::shared_ptr<IConnection> SocketIONetworkService::doAccept()
{
	auto conn = std::make_shared<SocketIOConnection>(acceptSock, protocol, remoteEndpoint, *this);
	activeConnections[acceptEndpoint.port] = conn;
	return conn;
}

void SocketIONetworkService::doReject()
{

}

std::shared_ptr<SocketIOConnection> SocketIONetworkService::doAcceptConnection(Endpoint endPoint)
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

void SocketIONetworkService::doRejectConnection()
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

void SocketIONetworkService::setEndpointFromAddrInfo(Endpoint& endpoint, uint16_t port, const addrinfo* addr)
{
	if (!addr || addr->ai_addrlen > sizeof(Endpoint)) {
		throw Exception("Invalid addrinfo", HalleyExceptions::Network);
	}

	memcpy(&endpoint, addr->ai_addr, addr->ai_addrlen);
	endpoint.port = port;
}

void SocketIONetworkService::setEndpointFromSockAddr(Endpoint& endpoint, const sockaddr* addr, int addrLen)
{
	if (addr->sa_family == AF_INET6) {
		Expects(addrLen == sizeof(sockaddr_in6));
		const auto addr6 = reinterpret_cast<const sockaddr_in6 *>(addr);
		endpoint.ipv6 = addr6->sin6_addr;
		endpoint.port = addr6->sin6_port;
	} else if (addr->sa_family == AF_INET) {
		Expects(addrLen == sizeof(sockaddr_in));
		const auto addr4 = reinterpret_cast<const sockaddr_in *>(addr);
		endpoint.ipv4.ip = addr4->sin_addr;
		endpoint.port = addr4->sin_port;
	} else {
		throw Exception("Unexpected addr family", HalleyExceptions::Network);
	}
}

void SocketIONetworkService::setSockAddrFromEndpoint(const Endpoint& endpoint, sockaddr* addr, int* addrLen)
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}
