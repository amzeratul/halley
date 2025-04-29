#include "socketio_network_service.h"

#include "halley/net/connection/network_packet.h"

#if defined(_WIN32)
#include <iphlpapi.h>
#pragma comment(lib, "Iphlpapi.lib")
#endif

using namespace Halley;

static bool isValidEndpoint(const Endpoint& endpoint, IPVersion version)
{
	if (endpoint.version != version) {
		return false;
	}

	if (endpoint.version == IPVersion::IPv6) {
		static in6_addr empty = {};
		return memcmp(&endpoint.addr.v6, &empty, sizeof(empty)) != 0;
	} else {
		static in_addr empty = {};
		return memcmp(&endpoint.addr.v4, &empty, sizeof(empty)) != 0;
	}
}

static String getEndpointAddress(const Endpoint& endpoint)
{
	bool isIPv6WithPort = endpoint.version == IPVersion::IPv6 && endpoint.port != 0;
	String result = isIPv6WithPort ? "[" : "";

	if (endpoint.version == IPVersion::IPv6) {
		char buf[INET6_ADDRSTRLEN + 1] = {};
		const char* ip = inet_ntop(AF_INET6, &endpoint.addr.v6, buf, INET6_ADDRSTRLEN);
		result += ip == buf ? ip : "::0";
	} else {
		char buf[INET_ADDRSTRLEN + 1] = {};
		const char* ip = inet_ntop(AF_INET, &endpoint.addr.v4, buf, INET_ADDRSTRLEN);
		result += ip == buf ? ip : "0.0.0.0";
	}

	if (isIPv6WithPort) {
		result += "]";
	}

	if (endpoint.port != 0) {
		result += ":" + toString(endpoint.port);
	}

	return result;
}

static void setEndpointFromAddrInfo(Endpoint& endpoint, const addrinfo* addr)
{
	endpoint.version = addr->ai_family == AF_INET6 ? IPVersion::IPv6 : IPVersion::IPv4;

	if (endpoint.version == IPVersion::IPv6) {
		const auto addr6 = reinterpret_cast<const sockaddr_in6 *>(addr->ai_addr);
		endpoint.addr.v6 = addr6->sin6_addr;
		endpoint.port = ntohs(addr6->sin6_port);
	} else {
		const auto addr4 = reinterpret_cast<const sockaddr_in *>(addr->ai_addr);
		endpoint.addr.v4 = addr4->sin_addr;
		endpoint.port = ntohs(addr4->sin_port);
	}
}

static void setEndpointFromSockAddr(Endpoint& endpoint, const sockaddr* addr, int addrLen)
{
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

static sockaddr* getSockAddrFromEndpoint(const Endpoint& endpoint, int* addrLen)
{
	thread_local sockaddr_storage storage;
	auto addr = reinterpret_cast<sockaddr*>(&storage);

	if (endpoint.version == IPVersion::IPv6) {
		const auto addr6 = reinterpret_cast<sockaddr_in6*>(addr);
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(endpoint.port);
		addr6->sin6_addr = endpoint.addr.v6;
		*addrLen = sizeof(sockaddr_in6);
	} else {
		const auto addr4 = reinterpret_cast<sockaddr_in*>(addr);
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(endpoint.port);
		addr4->sin_addr = endpoint.addr.v4;
		*addrLen = sizeof(sockaddr_in);
	}

	return addr;
}

static bool resolveIPOrDomain(const String& ipOrDomain, uint16_t port, const addrinfo* hints, Endpoint* ipv4, Endpoint* ipv6)
{
	int found = 0;

	addrinfo* result = nullptr;
	const int err = getaddrinfo(ipOrDomain.c_str(), port == 0 ? nullptr : toString(port).c_str(), hints, &result);

	if (err == 0) {
		// Scan list of results for IPv4 and IPv6 entries.
		for (const addrinfo* res = result; res != nullptr; res = res->ai_next) {
			if (res->ai_family == AF_INET && ipv4 != nullptr) {
				setEndpointFromAddrInfo(*ipv4, res);
				found++;
			} else if (res->ai_family == AF_INET6 && ipv6 != nullptr) {
				setEndpointFromAddrInfo(*ipv6, res);
				found++;
			}
		}
		freeaddrinfo(result);
	} else {
		Logger::logError("getaddrinfo() failed for '" + ipOrDomain + "' with error code " + toString(err));
	}

	return found > 0;
}

static bool resolveIP(const String& ip, Endpoint* ipv4, Endpoint* ipv6)
{
	addrinfo hints = {};
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	return resolveIPOrDomain(ip, 0, &hints, ipv4, ipv6);
}

static bool resolveDomain(const String& name, Endpoint* ipv4, Endpoint* ipv6)
{
	addrinfo hints = {};
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	return resolveIPOrDomain(name, 0, &hints, ipv4, ipv6);
}

static Endpoint selectAdapterForAddress(const Endpoint* hint, const Endpoint& remote)
{
	Endpoint localAddr = {};

	int remoteAddrLen;
	sockaddr* remoteAddr = getSockAddrFromEndpoint(remote, &remoteAddrLen);

#if defined(_WIN32)
	// Obtain list of existing network adapters.
	// TODO: this uses quite a piece of stack memory.
	IP_ADAPTER_ADDRESSES adapterAddresses[40] = {};
	{
		ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_INCLUDE_PREFIX;
		ULONG size = sizeof(adapterAddresses);

		ULONG err = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapterAddresses, &size);

		if (err != ERROR_SUCCESS) {
			Logger::logError("GetAdaptersAddresses() failed.");
	        return {};
		}
	}

    // Ask Windows to choose an adapter.
    DWORD bestIndex = 0;
    DWORD err = GetBestInterfaceEx(remoteAddr, &bestIndex);

    if (err == ERROR_NETWORK_UNREACHABLE) {
        Logger::logError("GetBestInterfaceEx() failed, network unreachable");
        return {};
    } else if (err != NO_ERROR) {
        Logger::logError("GetBestInterfaceEx() failed, error code: " + toString(err));
        return {};
    }

    // Search for the matching adapter address, by "best index".
    const IP_ADAPTER_ADDRESSES* addr = adapterAddresses;

	while (addr != nullptr) {
        const IF_INDEX index = (remoteAddr->sa_family == AF_INET) ? addr->IfIndex : addr->Ipv6IfIndex;
        if (index == bestIndex) {
            break;
        }
        addr = addr->Next;
    }

    // Extract the socket address.
    if (addr != nullptr) {
        /*
         * Scan list of unicast addresses, search for match by family.
         */
        const Endpoint* hintAddr = nullptr;
        if (hint != nullptr && hint->version == IPVersion::IPv6) {
            hintAddr = hint;
        }

        IP_ADAPTER_UNICAST_ADDRESS_LH* unicast = addr->FirstUnicastAddress;
        IP_ADAPTER_UNICAST_ADDRESS_LH* bestUnicast = nullptr;

        while (unicast != nullptr) {
            const sockaddr* unicastAddr = unicast->Address.lpSockaddr;
            if (unicastAddr->sa_family == remoteAddr->sa_family) {
                if (hintAddr != nullptr) {
                    if (bestUnicast == nullptr) {
                        bestUnicast = unicast;
                    } else if (unicastAddr->sa_family == AF_INET6) {
                        auto probeAddr = reinterpret_cast<const sockaddr_in6*>(unicastAddr);
                        if (memcmp(probeAddr->sin6_addr.u.Byte, hintAddr->addr.v6.u.Byte, 16)) {
                            bestUnicast = unicast;
                            break;
                        }
                    }
                } else {
                    bestUnicast = unicast;
                    break;
                }
            }
            unicast = unicast->Next;
        }

        // Copy address to own structure.
        if (bestUnicast != nullptr) {
        	setEndpointFromSockAddr(localAddr, bestUnicast->Address.lpSockaddr, bestUnicast->Address.iSockaddrLength);
        }
    }

#else
	throw Exception("Not implemented", HalleyExceptions::Network);
#endif

	localAddr.port = 0;

    return localAddr;
}

static Endpoint selectAdapterForAddress(const Endpoint& ipv4, const Endpoint& ipv6, const Endpoint* hint, bool ipv4Only)
{
	Endpoint ipv4Local = {}, ipv6Local = {};

	// Windows-specific: see comment in function above.
	if (!ipv4Only && isValidEndpoint(ipv6, IPVersion::IPv6)) {
		if (hint != nullptr && isValidEndpoint(*hint, IPVersion::IPv6)) {
			ipv6Local = selectAdapterForAddress(hint, ipv6);
		} else {
			ipv6Local = selectAdapterForAddress(nullptr, ipv6);
		}
	}

	if (isValidEndpoint(ipv4, IPVersion::IPv4)) {
		ipv4Local = selectAdapterForAddress(nullptr, ipv4);
	}

	if (isValidEndpoint(ipv6Local, IPVersion::IPv6)) {
		return ipv6Local;
	}

	if (isValidEndpoint(ipv4Local, IPVersion::IPv4)) {
		return ipv4Local;
	}

	return {};
}

static Endpoint selectAdapterForIP(const String& ip, bool ipv4Only)
{
	Endpoint ipv4 = {}, ipv6 = {};
	if (!resolveIP(ip, &ipv4, &ipv6)) {
		return {};
	}

	return selectAdapterForAddress(ipv4, ipv6, &ipv6, ipv4Only);
}

static Endpoint selectAdapterForDomain(const String& name, bool ipv4Only)
{
	Endpoint ipv4 = {}, ipv6 = {};
	if (!resolveDomain(name, &ipv4, &ipv6)) {
		return {};
	}

	return selectAdapterForAddress(ipv4, ipv6, nullptr, ipv4Only);
}

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
		if (!disconnectReason.isEmpty()) {
			Logger::logError("Closing " + toString(protocol == NetworkProtocol::TCP ? "TCP" : "UDP") + " connection: " + disconnectReason);
		}
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
			disconnect("failed to send data on TCP socket");
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
			disconnect("invalid inbound packet size");
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
	Expects(status == ConnectionStatus::Connected);

	{
		fd_set rd = {1};
		rd.fd_array[0] = sock;
		constexpr timeval timeout = {0, 0};

		const int err = ::select(0, &rd, nullptr, nullptr, &timeout);

		if (err == 0) {
			return; // No pending data.
		}

		if (err == SOCKET_ERROR) {
			disconnect("error receiving data on TCP socket");
			return;
		}

		if (err != 1) {
			Logger::logWarning("Unexpected TCP socket read status: " + toString(err));
		}
	}

	while (status == ConnectionStatus::Connected) {
		std::array<char, 4096> buffer;
		constexpr int bufferSize = static_cast<int>(buffer.size());
		const int bytesRecv = ::recv(sock, buffer.data(), bufferSize, 0);

		if (bytesRecv == 0) {
			break; // "If the connection has been gracefully closed, the return value is zero."
		}

		if (bytesRecv == SOCKET_ERROR) {
			const int err = WSAGetLastError();

			if (err == WSAEWOULDBLOCK) {
				break;
			}

			if (err == WSAEMSGSIZE) {
				throw Exception("Not implemented", HalleyExceptions::Network);
			}

			disconnect("error receiving data on socket");
			break;
		}

		size_t curQueueSize = receiveQueue.size();
		receiveQueue.resize(curQueueSize + bytesRecv);
		memcpy(receiveQueue.data() + curQueueSize, buffer.data(), bytesRecv);

		if (bytesRecv < bufferSize) {
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

String SocketIOConnection::getRemoteAddress() const
{
	return getEndpointAddress(remote);
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

	int addrLen;
	const auto* addr = getSockAddrFromEndpoint(remote, &addrLen);

	const int bytesSent = ::sendto(sock, reinterpret_cast<const char *>(packet.data()), len, 0, addr, addrLen);

	if (bytesSent == SOCKET_ERROR || bytesSent != len) {
		int err = WSAGetLastError();
		disconnect("Error sending data on UDP socket: " + toString(err));
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

void SocketIOConnection::disconnect(const String& reason)
{
	if (status == ConnectionStatus::Connected) {
		disconnectReason = reason;
		status = ConnectionStatus::Closing;
	}
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

	// We don't know who we are going to talk to, so choose a network adapter
	// by probing a well-known public address.
	// - 1.1.1.1 / 2606:4700:4700::1111 are Cloudflare public DNS servers.
	{
		if (localEndpoint.version == IPVersion::IPv6) {
			const Endpoint adapter = selectAdapterForIP("2606:4700:4700::1111", false);
			Expects(isValidEndpoint(adapter, IPVersion::IPv6));
			localEndpoint.addr.v6 = adapter.addr.v6;
		} else {
			const Endpoint adapter = selectAdapterForIP("1.1.1.1", true);
			Expects(isValidEndpoint(adapter, IPVersion::IPv4));
			localEndpoint.addr.v4 = adapter.addr.v4;
		}
	}

	acceptCallback = std::move(callback);

	// Create local socket.
	const int sockFamily = localEndpoint.version == IPVersion::IPv6 ? AF_INET6 : AF_INET;
	const int sockType = protocol == NetworkProtocol::TCP ? SOCK_STREAM : SOCK_DGRAM;
	const int sockProtocol = protocol == NetworkProtocol::TCP ? IPPROTO_TCP : IPPROTO_UDP;

#ifdef _WIN32
	const SOCKET s = socket(sockFamily, sockType, sockProtocol);

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
	int bindAddrLen;
	const auto bindAddr = getSockAddrFromEndpoint(localEndpoint, &bindAddrLen);

	if (bind(sock, bindAddr, bindAddrLen) == 0) {
		Logger::logInfo("Listening at " + getEndpointAddress(localEndpoint));
	} else {
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
	sockaddr_storage remoteAddrStorage = {};
	auto remoteAddr = reinterpret_cast<sockaddr*>(&remoteAddrStorage);
	int remoteAddrLen = sizeof(remoteAddrStorage);

	Socket acceptSock = -1;

#ifdef _WIN32
	SOCKET a = ::accept(sock, remoteAddr, &remoteAddrLen);

	if (a == INVALID_SOCKET) {
		throw Exception("Failed to accept incoming request", HalleyExceptions::Network);
	}

	acceptSock = a;
#else
	throw Exception("Not implemented", HalleyExceptions::Network);
#endif

	setEndpointFromSockAddr(remoteEndpoint, remoteAddr, remoteAddrLen);

	if (acceptCallback) {
		SocketAcceptor acceptor(*this, acceptSock, remoteEndpoint);
		acceptCallback(acceptor);
		acceptor.ensureChoiceMade();
	}
}

std::shared_ptr<IConnection> SocketIONetworkService::connect(const String& address)
{
	String ip;
	uint16_t port = 0;

	// Try to parse the address as IPv4, IPv6 or domain name.
	// This is very basic, and probably very terrible.
	if (address.contains('.')) {
		// IPv4
		const size_t colon = address.find_last_of(':');
		if (colon == String::npos) {
			ip = address;
		} else {
			ip = address.substr(0, colon);
			port = address.subToInteger(colon + 1, address.length());
		}
	} else if (address[0] == '[' || std::isdigit(address[0])) {
		// IPv6
		if (address[0] != '[') {
			ip = address;
		} else {
			const size_t colon = address.find_last_of(':');
			if (colon == String::npos || address[colon - 1] != ']') {
				throw Exception("Malformed IPv6 string", HalleyExceptions::Network);
			}
			ip = address.substr(1, colon - 2);
			port = address.subToInteger(colon + 1, address.length());
		}
	} else {
		// A domain name
		throw Exception("Not implemented", HalleyExceptions::Network);
	}

	// Resolve the connect IP to figure which adapter to use.
	{
		Endpoint adapter = selectAdapterForIP(ip, localEndpoint.version == IPVersion::IPv4);

		if (localEndpoint.version == IPVersion::IPv6) {
			Expects(isValidEndpoint(adapter, IPVersion::IPv6));
			localEndpoint.addr.v6 = adapter.addr.v6;
		} else {
			Expects(isValidEndpoint(adapter, IPVersion::IPv4));
			localEndpoint.addr.v4 = adapter.addr.v4;
		}
	}

	// Create the local socket.
	const int sockFamily = localEndpoint.version == IPVersion::IPv6 ? AF_INET6 : AF_INET;
	const int sockType = protocol == NetworkProtocol::TCP ? SOCK_STREAM : SOCK_DGRAM;
	const int sockProtocol = protocol == NetworkProtocol::TCP ? IPPROTO_TCP : IPPROTO_UDP;

#if defined(_WIN32) || defined(WITH_GDK)
	const SOCKET s = socket(sockFamily, sockType, sockProtocol);

	if (s == INVALID_SOCKET) {
		throw Exception("Failed to create local socket", HalleyExceptions::Network);
	}

	sock = s;
#else
	throw Exception("Not implemented", HalleyExceptions::Network);
#endif

	if (localEndpoint.version == IPVersion::IPv6) {
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

	// Resolve connect IP again to get the remote address.
	if (localEndpoint.version == IPVersion::IPv6) {
		resolveIP(ip, nullptr, &remoteEndpoint);
		remoteEndpoint.port = port;
	} else {
		resolveIP(ip, &remoteEndpoint, nullptr);
		remoteEndpoint.port = port;
	}

	// On *some* platforms, we want to bind to a preferred port.
	if (localEndpoint.port != 0) {
		int bindAddrLen;
		const auto bindAddr = getSockAddrFromEndpoint(localEndpoint, &bindAddrLen);

		if (bind(sock, bindAddr, bindAddrLen) != 0) {
			throw Exception("Failed to bind local socket", HalleyExceptions::Network);
		}
	}

	// Connect.
	if (protocol == NetworkProtocol::TCP) {
		int remoteAddrLen;
		const auto remoteAddr = getSockAddrFromEndpoint(remoteEndpoint, &remoteAddrLen);

		if (::connect(sock, remoteAddr, remoteAddrLen) != 0) {
			Logger::logError("Failed to connect socket to " + address);
			return {};
		}
	}

	// Create and keep the connection interface.
	Logger::logInfo("Connecting " + getEndpointAddress(localEndpoint) + " to " + getEndpointAddress(remoteEndpoint));

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
			SocketAcceptor acceptor(*this, sock, endpoint);
			acceptCallback(acceptor);
			acceptor.ensureChoiceMade();
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
		} catch (...) {
			//connection->setError("Unknown error receiving packet.");
			connection->close();
		}
	}
}

bool SocketIONetworkService::hasConnectionWithId(short connId) const
{
    return activeConnections.find(connId) != activeConnections.end();
}

std::shared_ptr<SocketIOConnection> SocketIONetworkService::doAcceptConnection(Socket socket, const Endpoint& endpoint)
{
	std::shared_ptr<SocketIOConnection> conn;

	if (protocol == NetworkProtocol::TCP) {
		conn = std::make_shared<SocketIOConnection>(socket, protocol, endpoint, *this);
		activeConnections[endpoint.port] = conn;
	} else {
		conn = std::make_shared<SocketIOConnection>(socket, protocol, endpoint, *this);

		const short id = getFreeConnectionId();

		sendHandshakeAccept(*conn, id);
		conn->onConnect(id);

		activeConnections[id] = conn;
	}

	return conn;
}

void SocketIONetworkService::doRejectConnection()
{
	// TODO: nothing to do here?
}

SocketIONetworkService::SocketAcceptor::SocketAcceptor(SocketIONetworkService& service, Socket socket, const Endpoint& endpoint)
	: service(service)
	, sock(socket)
	, endpoint(std::move(endpoint))
{

}

std::shared_ptr<IConnection> SocketIONetworkService::SocketAcceptor::doAccept()
{
	return service.doAcceptConnection(sock, endpoint);
}

void SocketIONetworkService::SocketAcceptor::doReject()
{
	service.doRejectConnection();
}
