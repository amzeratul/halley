#include "halley/data_structures/vector.h"
#include "halley/net/connection/network_packet.h"
#include "halley/net/connection/instability_simulator.h"
#include "asio_udp_network_service.h"
#include "asio_udp_connection.h"
#include <iostream>

using namespace Halley;

AsioUDPNetworkService::AsioUDPNetworkService(int port, IPVersion version)
	: localEndpoint(version == IPVersion::IPv4 ? asio::ip::udp::v4() : asio::ip::udp::v6(), static_cast<unsigned short>(port))
	, socket(service, localEndpoint)
{
	HalleyAssertDev(port == 0 || port > 1024);
	HalleyAssertDev(port < 65536);

    // Set to non-blocking!
    socket.non_blocking(true);
}


AsioUDPNetworkService::~AsioUDPNetworkService()
{
	for (auto& conn : activeConnections) {
		try {
			conn.second->terminateConnection();
		} catch (...) {
			std::cout << "Error terminating connection on ~NetworkService()" << std::endl;
		}
	}
	try {
		service.poll();
		socket.shutdown(UDPSocket::shutdown_both);
	} catch (...) {
		std::cout << "Error polling service on ~NetworkService()" << std::endl;
	}
}

void AsioUDPNetworkService::update(Time t)
{
	NetworkServiceWithStats::update(t);
	
	// Remove closed connections
	Vector<short> toErase;
	auto& active = activeConnections;
	for (auto& conn: active) {
		if (conn.second->getStatus() == ConnectionStatus::Closing) {
			conn.second->terminateConnection();
			toErase.push_back(conn.first);
		}
	}

	for (auto i: toErase) {
		active.erase(i);
	}

	// Update service
	service.poll();

    auto callback = [this](UDPEndpoint& remote, gsl::span<std::byte> packet) {
        std::string* errorMsgPtr = nullptr;
        receivePacket(remote, packet, errorMsgPtr);
    };

    AsioUDPConnection::receiveAll(socket, activeConnections, callback);
}

std::shared_ptr<IConnection> AsioUDPNetworkService::connect(const String& address)
{
	const auto splitAddr = address.split(':');
	const auto addr = splitAddr.at(0);
	const auto port = splitAddr.at(1).toInteger();
	
	HalleyAssertDev(port > 1024);
	HalleyAssertDev(port < 65536);
	auto remoteAddr = asio::ip::make_address(addr.cppStr());
	auto remote = UDPEndpoint(remoteAddr, static_cast<unsigned short>(port)); 
	auto conn = std::make_shared<AsioUDPConnection>(socket, remote);
	activeConnections[0] = conn;

	// Handshake
    sendHandshake(*conn);
    acceptCallback = {};

	return conn;
}

String AsioUDPNetworkService::startListening(AcceptCallback callback)
{
	acceptCallback = std::move(callback);
	if (!startedListening) {
		startedListening = true;
	}
	return "";
}

void AsioUDPNetworkService::stopListening()
{
	acceptCallback = {};
}

void AsioUDPNetworkService::receivePacket(UDPEndpoint& endpoint, gsl::span<std::byte> received, std::string* error)
{
	if (error) {
		std::cout << "Error receiving packet: " << (*error) << std::endl;
		// Find the owner of this remote endpoint
		for (auto& conn : activeConnections) {
			if (conn.second->matchesEndpoint(endpoint)) {
				conn.second->setError(*error);
				conn.second->close();
			}
		}
		return;
	}

	if (received.size_bytes() == 0) {
		return;
	}

	// Read connection id
	short id = -1;
	std::array<unsigned char, 2> bytes;
	auto dst = gsl::as_writable_bytes(gsl::span<unsigned char, 2>(bytes));
	dst[0] = received[0];
	if (bytes[0] & 0x80) {
		if (received.size_bytes() < 2) {
			// Invalid header
			std::cout << "Invalid header\n";
			return;
		}
		dst[1] = received[1];
		received = received.subspan(2);
		id = short((bytes[0] & 0x7F) << 8) | short(bytes[1]);
	} else {
		received = received.subspan(1);
		id = short(bytes[0]);
	}

	// No connection id, check if it's a connection request
	if (id == 0 && isValidHandshake(received, nullptr)) {
        if (acceptCallback) {
            auto a = UDPAcceptor(*this, endpoint);
            if (acceptCallback) {
                acceptCallback(a);
            }
            a.ensureChoiceMade();
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

		if (error) {
			// Close the connection if there was an error
			connection->setError(*error);
			connection->close();
		} else {
			try {
				if (conn->first == 0) {
                    // Hold on, we're still on 0, re-bind to the id
                    HalleyAssertDev(id != 0);
                    connection->onConnect(id);

                    activeConnections[id] = connection;
                    activeConnections.erase(conn);
				}
			} catch (std::exception& e) {
				connection->setError(e.what());
				connection->close();
			} catch (...) {
				connection->setError("Unknown error receiving packet.");
				connection->close();
			}
		}
	}
}

bool AsioUDPNetworkService::hasConnectionWithId(short connId) const
{
    return activeConnections.find(connId) != activeConnections.end();
}

std::shared_ptr<AsioUDPConnection> AsioUDPNetworkService::doAcceptConnection(UDPEndpoint endPoint)
{
	auto conn = std::make_shared<AsioUDPConnection>(socket, std::move(endPoint));
	short id = getFreeConnectionId();

    sendHandshakeAccept(*conn, id);
    conn->onConnect(id);

	activeConnections[id] = conn;
	return conn;
}

void AsioUDPNetworkService::doRejectConnection()
{
	
}

AsioUDPNetworkService::UDPAcceptor::UDPAcceptor(AsioUDPNetworkService& service, UDPEndpoint endPoint)
	: service(service)
	, endPoint(std::move(endPoint))
{}

std::shared_ptr<IConnection> AsioUDPNetworkService::UDPAcceptor::doAccept()
{
	return service.doAcceptConnection(endPoint);
}

void AsioUDPNetworkService::UDPAcceptor::doReject()
{
	return service.doRejectConnection();
}
