#include "socketio_network_service.h"

#include "halley/net/connection/network_packet.h"

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
		// UDP connections do not "own" the socket, so don't close it here.
		if (sock >= 0 && protocol == NetworkProtocol::TCP) {
#ifdef _WIN32
			closesocket(sock);
#else
			close(sock);
#endif
		}

		status = ConnectionStatus::Closed;
	}
}

void SocketIOConnection::update()
{
	if (status == ConnectionStatus::Closing) {
		close();
	}

	if (protocol == NetworkProtocol::TCP && status == ConnectionStatus::Connected) {
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
	if (remote.version != remoteEndpoint.version) {
		return false;
	}

	if (remote.port != remoteEndpoint.port) {
		return false;
	}

	if (remote.version == IPVersion::IPv6) {
		return memcmp(&remote.addr.v6, &remoteEndpoint.addr.v6, sizeof(in6_addr)) == 0;
	} else {
		return memcmp(&remote.addr.v4, &remoteEndpoint.addr.v4, sizeof(in_addr)) == 0;
	}
}

void SocketIOConnection::terminateConnection()
{
	throw Exception("Not implemented", HalleyExceptions::Network);
}

String SocketIOConnection::getRemoteAddress() const
{
	sockaddr addr = {};
	int addrLen;
	setSockAddrFromEndpoint(remote, &addr, &addrLen);

	char node[NI_MAXHOST] = {};
	char serv[NI_MAXSERV] = {};

	const int err = getnameinfo(&addr, addrLen, node, sizeof(node), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);

	if (err == 0) {
		return String(node) + ":" + String(serv);
	}

	return IConnection::getRemoteAddress();
}

size_t SocketIOConnection::getMaxUnreliablePacketSize() const
{
	return 1384;
}

void SocketIOConnection::onConnect(short connId)
{
	if (status == ConnectionStatus::Connecting) {
		Logger::logDev("Connection established as id = " + toString(connId));
		connectionId = connId;
		status = ConnectionStatus::Connected;
	}
}

void SocketIOConnection::sendUnreliablePacket(gsl::span<const gsl::byte> packet)
{
	if (status != ConnectionStatus::Connected && status != ConnectionStatus::Connecting) {
		Logger::logError("Attempting to send packet, but not in connected state", true);
		return;
	}

	const int len = static_cast<int>(packet.size_bytes());

	sockaddr_storage addrStore = {};
	sockaddr* addr = reinterpret_cast<sockaddr *>(&addrStore);
	int addrLen = sizeof(addrStore);
	setSockAddrFromEndpoint(remote, addr, &addrLen);

	const int bytesSent = ::sendto(sock, reinterpret_cast<const char *>(packet.data()), len, 0, addr, addrLen);

	if (bytesSent == SOCKET_ERROR || bytesSent != len) {
		int err = WSAGetLastError();
		Logger::logError("Error sending data on UDP socket: " + toString(err));
		close();
		return;
	}

	if (packetListener != nullptr) {
		packetListener->onSend(packet);
	}
}

void SocketIOConnection::receiveUnreliablePacket(gsl::span<const gsl::byte> packet) const
{
	if (packetListener != nullptr) {
		packetListener->onReceive(packet);
	} else {
		Logger::logError("No packet listener registered, packet will be lost", true);
	}
}

void SocketIOConnection::setUnreliablePacketListener(IPacketListener* listener)
{
	packetListener = listener;
}

SocketIONetworkService::SocketIONetworkService(int port, NetworkProtocol protocol, IPVersion version)
	: protocol(protocol)
{
	Expects(port == 0 || port > 1024);
	Expects(port < 65536);
	localEndpoint.port = static_cast<uint16_t>(port);
	localEndpoint.version = version;
}

SocketIONetworkService::~SocketIONetworkService()
{
	SocketIONetworkService::stopListening();
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
	tryReceiveUnreliable();
}

String SocketIONetworkService::startListening(AcceptCallback callback)
{
	Ensures(!startedListening);

	acceptCallback = std::move(callback);

	// Create local socket.
	int sockType = protocol == NetworkProtocol::TCP ? SOCK_STREAM : SOCK_DGRAM;
	int sockProtocol = protocol == NetworkProtocol::TCP ? IPPROTO_TCP : IPPROTO_UDP;

#ifdef _WIN32
	const SOCKET s = socket(AF_INET, sockType, sockProtocol);

	if (s == INVALID_SOCKET) {
		throw Exception("Failed to create local socket", HalleyExceptions::Network);
	}

	sock = s;
#else
	throw Exception("Not implemented", HalleyExceptions::Network);
#endif

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

	// Bind to local port.
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(localEndpoint.port);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0) {
		throw Exception("Failed to bind local socket", HalleyExceptions::Network);
	}

	if (protocol == NetworkProtocol::TCP) {
		// Start to listen.
		if (listen(sock, 1) == SOCKET_ERROR) {
			throw Exception("Failed to listen on local socket", HalleyExceptions::Network);
		}
	}

	startedListening = true;

	return "";
}

void SocketIONetworkService::stopListening()
{
	if (!startedListening) {
		return;
	}

	acceptCallback = {};
	startedListening = false;

	// For TCP services, this closes the listen socket.
	// For UDP, this closes the one and only DGRAM socket.
	if (sock >= 0) {
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		sock = -1;
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
	size_t lastDelim = address.find_last_of(':');
	Ensures(lastDelim != String::npos);
	const auto ip = address.substr(0, lastDelim);
	const auto& portAsString = address.substr(lastDelim + 1);
	const uint16_t port = portAsString.toInteger();

	// Figure out socket parameters for connecting to remote address.
	addrinfo hint = {};
	hint.ai_flags = 0;
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = protocol == NetworkProtocol::TCP ? SOCK_STREAM : SOCK_DGRAM;

	addrinfo* result;
	if (getaddrinfo(ip.c_str(), portAsString.c_str(), &hint, &result) != 0) {
		Logger::logError("Failed to create local socket to connect to " + address);
		return {};
	}

	// Create the local socket.
#if defined(_WIN32) || defined(WITH_GDK)
	const SOCKET s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (s == INVALID_SOCKET) {
		throw Exception("Failed to create local socket", HalleyExceptions::Network);
	}

	sock = s;
#else
	throw Exception("Not implemented", HalleyExceptions::Network);
#endif

	if (result->ai_family == AF_INET6) {
		int ipv6only = 1;
		if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&ipv6only), sizeof(ipv6only)) == SOCKET_ERROR) {
			throw Exception("Failed to set IPv6-only on local socket", HalleyExceptions::Network);
		}
	}

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
	if (protocol == NetworkProtocol::TCP) {
		if (::connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen)) != 0) {
			Logger::logError("Failed to connect socket to " + address);
			freeaddrinfo(result);
			return {};
		}
	}

	// Need to free up this.
	freeaddrinfo(result);

	// Obtain the local address
	sockaddr_storage localAddrStorage = {};
	auto localAddr = reinterpret_cast<sockaddr*>(&localAddrStorage);
	int localAddrLen = localEndpoint.version == IPVersion::IPv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
	if (getsockname(sock, localAddr, &localAddrLen) == 0) {
		setEndpointFromSockAddr(localEndpoint, localAddr, localAddrLen);
	} else {
		Logger::logError("Failed to get local socket name");
	}

	auto conn = std::make_shared<SocketIOConnection>(sock, protocol, remoteEndpoint, *this);
	activeConnections[0] = conn;

	// Handshake
	if (protocol == NetworkProtocol::UDP) {
		sendHandshake(*conn);
	}

	return conn;
}

void SocketIONetworkService::tryReceiveUnreliable()
{
	if (protocol != NetworkProtocol::UDP) {
		return;
	}

	// Poll until there's no more data.
	std::array<char, 2048> buffer;

	for (;;) {
		sockaddr_storage addrStorage = {};
		auto addr = reinterpret_cast<sockaddr*>(&addrStorage);
		int addrLen = localEndpoint.version == IPVersion::IPv6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);

		const int bytesRecv = ::recvfrom(sock, buffer.data(), (int) buffer.size(), 0, addr, &addrLen);

		if (bytesRecv == 0) {
			break;
		}

		Endpoint remote = {};
		setEndpointFromSockAddr(remote, addr, addrLen);

		short activeConnId = -1;
		for (const auto& active : activeConnections) {
			if (active.second->getStatus() == ConnectionStatus::Connected && active.second->matchesEndpoint(remote)) {
				activeConnId = active.first;
				break;
			}
		}

		if (bytesRecv == SOCKET_ERROR) {
			const int err = WSAGetLastError();

			if (err == WSAEWOULDBLOCK) {
				break; // No more data available, stop polling.
			}

			if (err == WSAECONNRESET) {
				// Connection closed by remote host.
				if (activeConnId >= 0) {
					activeConnections[activeConnId]->close();
					Logger::logWarning("Connection " + toString(activeConnId) + " reset by remote host");
				}
				break;
			}

			// TODO: might need more checks by error code
			Logger::logError("Error receiving UDP packet: " + toString(err));
			break;
		}

		auto packet = gsl::span(reinterpret_cast<gsl::byte *>(buffer.data()), bytesRecv);

		if (activeConnId >= 0) {
			activeConnections[activeConnId]->receiveUnreliablePacket(packet);
		} else {
			receivePacket(remote, packet);
		}
	}
}

void SocketIONetworkService::receivePacket(const Endpoint& endpoint, gsl::span<gsl::byte> data)
{
	if (data.size_bytes() == 0) {
		return;
	}

	// Read connection id
	short id = -1;
	std::array<unsigned char, 2> bytes;
	const auto dst = gsl::as_writable_bytes(gsl::span(bytes));
	dst[0] = data[0];
	if (bytes[0] & 0x80) {
		if (data.size_bytes() < 2) {
			// Invalid header
			std::cout << "Invalid header\n";
			return;
		}
		dst[1] = data[1];
		data = data.subspan(2);
		id = static_cast<short>((bytes[0] & 0x7F) << 8) | static_cast<short>(bytes[1]);
	} else {
		data = data.subspan(1);
		id = static_cast<short>(bytes[0]);
	}

	// No connection id, check if it's a connection request
	if (id == 0 && isValidHandshake(data, nullptr)) {
		if (acceptCallback) {
			acceptEndpoint = endpoint;
			if (acceptCallback) {
				acceptCallback(*this);
			}
			ensureChoiceMade();
			return;
		}
	}

	// Find the owner of this remote endpoint
	auto conn = activeConnections.find(id);
	if (conn == activeConnections.end()) {
		// Connection doesn't exist, but check the pending slot
		conn = activeConnections.find(0);
		if (conn == activeConnections.end()) {
			// Nope, give up
			return;
		}
	}

	// Validate that this connection is who it claims to be
	if (conn->second->matchesEndpoint(endpoint)) {
		auto connection = conn->second;

		try {
			if (conn->first == 0) {
				// Hold on, we're still on 0, re-bind to the id
				Expects(id != 0);
				connection->onConnect(id);

				activeConnections[id] = connection;
				activeConnections.erase(conn);
			}
		/*} catch (std::exception& e) {
			//connection->setError(e.what());
			connection->close();
		*/} catch (...) {
			//connection->setError("Unknown error receiving packet.");
			connection->close();
		}
	}
}

bool SocketIONetworkService::hasConnectionWithId(short connId) const
{
    return activeConnections.find(connId) != activeConnections.end();
}

std::shared_ptr<IConnection> SocketIONetworkService::doAccept()
{
	std::shared_ptr<SocketIOConnection> conn;

	if (protocol == NetworkProtocol::TCP) {
		conn = std::make_shared<SocketIOConnection>(acceptSock, protocol, remoteEndpoint, *this);
		activeConnections[acceptEndpoint.port] = conn;
	} else {
		conn = std::make_shared<SocketIOConnection>(sock, protocol, acceptEndpoint, *this);

		const short id = getFreeConnectionId();

		sendHandshakeAccept(*conn, id);
		conn->onConnect(id);

		activeConnections[id] = conn;
	}

	return conn;
}

void SocketIONetworkService::doReject()
{

}

void SocketIONetworkService::setEndpointFromAddrInfo(Endpoint& endpoint, uint16_t port, const addrinfo* addr)
{
	endpoint = {};
	endpoint.version = addr->ai_family == AF_INET6 ? IPVersion::IPv6 : IPVersion::IPv4;
	endpoint.port = port;

	if (endpoint.version == IPVersion::IPv6) {
		const auto addr6 = reinterpret_cast<const sockaddr_in6 *>(addr->ai_addr);
		endpoint.addr.v6 = addr6->sin6_addr;
	} else {
		const auto addr4 = reinterpret_cast<const sockaddr_in *>(addr->ai_addr);
		endpoint.addr.v4 = addr4->sin_addr;
	}
}

void SocketIONetworkService::setEndpointFromSockAddr(Endpoint& endpoint, const sockaddr* addr, int addrLen)
{
	endpoint = {};
	endpoint.version = addr->sa_family == AF_INET6 ? IPVersion::IPv6 : IPVersion::IPv4;

	if (endpoint.version == IPVersion::IPv6) {
		Expects(addrLen >= sizeof(sockaddr_in6));
		const auto addr6 = reinterpret_cast<const sockaddr_in6 *>(addr);
		endpoint.addr.v6 = addr6->sin6_addr;
		endpoint.port = ntohs(addr6->sin6_port);
	} else {
		Expects(addrLen >= sizeof(sockaddr_in));
		const auto addr4 = reinterpret_cast<const sockaddr_in *>(addr);
		endpoint.addr.v4 = addr4->sin_addr;
		endpoint.port = ntohs(addr4->sin_port);
	}
}

bool SocketIONetworkService::selectAdapterForAddress(const sockaddr* hint, sockaddr* addr, int* addrLen)
{
	return false;
}

void Halley::setSockAddrFromEndpoint(const Endpoint& endpoint, sockaddr* addr, int* addrLen)
{
	if (endpoint.version == IPVersion::IPv6) {
		Expects(*addrLen >= sizeof(sockaddr_in6));
		auto addr6 = reinterpret_cast<sockaddr_in6*>(addr);
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(endpoint.port);
		addr6->sin6_addr = endpoint.addr.v6;
		*addrLen = sizeof(sockaddr_in6);
	} else {
		Expects(*addrLen >= sizeof(sockaddr_in));
		auto addr4 = reinterpret_cast<sockaddr_in*>(addr);
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(endpoint.port);
		addr4->sin_addr = endpoint.addr.v4;
		*addrLen = sizeof(sockaddr_in);
	}
}
