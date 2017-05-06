#include <vector>
#include <list>
#include "halley/net/network_packet.h"
#include "asio_network_service.h"
#include "asio_udp_connection.h"
#include <iostream>
#include <unordered_map>
#include <halley/support/exception.h>

using namespace Halley;
namespace asio = boost::asio;


struct HandshakeOpen
{
	HandshakeOpen()
		: handshake("halley_open")
	{
	}

	const char handshake[12];
	// TODO: public-key encrypted temporary key
};



AsioNetworkService::AsioNetworkService(int port, IPVersion version)
	: localEndpoint(version == IPVersion::IPv4 ? asio::ip::udp::v4() : asio::ip::udp::v6(), port)
	, socket(service, localEndpoint)
{
	Expects(port == 0 || port > 1024);
	Expects(port < 65536);
}


AsioNetworkService::~AsioNetworkService()
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

void AsioNetworkService::update()
{
	// Remove closed connections
	std::vector<short> toErase;
	auto& active = activeConnections;
	for (auto& conn: active) {
		if (conn.second->getStatus() == ConnectionStatus::CLOSING) {
			conn.second->terminateConnection();
			toErase.push_back(conn.first);
		}
	}

	for (auto i: toErase) {
		active.erase(i);
	}

	// Update service
	service.poll();
}

void AsioNetworkService::setAcceptingConnections(bool accepting)
{
	acceptingConnections = accepting;
	if (accepting) {
		startListening();
	} else {
		pendingIncomingConnections.clear();
	}
}

std::shared_ptr<IConnection> AsioNetworkService::tryAcceptConnection()
{
	auto& pending = pendingIncomingConnections;

	if (pending.empty()) {
		return nullptr;
	} else {
		auto conn = std::make_shared<AsioUDPConnection>(socket, pending.front());
		short id = getFreeId();
		conn->open(id);

		activeConnections[id] = conn;
		pending.pop_front();
		return conn;
	}
}

std::shared_ptr<IConnection> AsioNetworkService::connect(String addr, int port)
{
	Expects(port > 1024);
	Expects(port < 65536);
	auto remoteAddr = asio::ip::address::from_string(addr.cppStr());
	auto remote = UDPEndpoint(remoteAddr, port); 
	auto conn = std::make_shared<AsioUDPConnection>(socket, remote);
	activeConnections[0] = conn;

	// Handshake
	HandshakeOpen open;
	conn->send(OutboundNetworkPacket(gsl::as_bytes(gsl::span<HandshakeOpen>(&open, 1))));

	startListening();

	return conn;
}

void AsioNetworkService::startListening()
{
	if (!startedListening) {
		startedListening = true;
		receiveNext();
	}
}

void AsioNetworkService::receiveNext()
{
	auto buffer = asio::buffer(receiveBuffer);
	socket.async_receive_from(buffer, remoteEndpoint, [this] (const boost::system::error_code& error, size_t size)
	{
		try {
			Expects(size <= receiveBuffer.size());

			std::string errorMsg;
			std::string* errorMsgPtr = nullptr;
			if (error) {
				errorMsg = error.message();
				errorMsgPtr = &errorMsg;
			}

			receivePacket(gsl::span<gsl::byte>(receiveBuffer.data(), size), errorMsgPtr);
		} catch (...) {
			std::cout << "Exception while receiving a packet." << std::endl;
		}

		receiveNext();
	});
}

void AsioNetworkService::receivePacket(gsl::span<gsl::byte> received, std::string* error)
{
	if (error) {
		std::cout << "Error receiving packet: " << (*error) << std::endl;
		// Find the owner of this remote endpoint
		for (auto& conn : activeConnections) {
			if (conn.second->matchesEndpoint(remoteEndpoint)) {
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
	auto dst = gsl::as_writeable_bytes(gsl::span<unsigned char, 2>(bytes));
	dst[0] = received[0];
	if (bytes[0] & 0x80) {
		if (received.size_bytes() < 2) {
			// Invalid header
			std::cout << "Invalid header\n";
			return;
		}
		dst[1] = received[1];
		received = received.subspan(2);
		id = short(bytes[0] & 0x7F) | short(bytes[1]);
	} else {
		received = received.subspan(1);
		id = short(bytes[0]);
	}

	// No connection id, check if it's a connection request
	if (id == 0 && isValidConnectionRequest(received)) {
		auto& pending = pendingIncomingConnections;
		if (std::find(pending.begin(), pending.end(), remoteEndpoint) == pending.end()) {
			pending.push_back(remoteEndpoint);
		}
		// Pending connection is valid
		return;
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
	if (conn->second->matchesEndpoint(remoteEndpoint)) {
		auto connection = conn->second;

		if (error) {
			// Close the connection if there was an error
			connection->setError(*error);
			connection->close();
		} else {
			try {
				connection->onReceive(received);

				if (conn->first == 0) {
					// Hold on, we're still on 0, re-bind to the id
					short newId = connection->getConnectionId();
					if (newId != 0) {
						activeConnections[newId] = connection;
						activeConnections.erase(conn);
					}
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

bool AsioNetworkService::isValidConnectionRequest(gsl::span<const gsl::byte> data)
{
	HandshakeOpen open;
	return acceptingConnections && data.size() == sizeof(open) && memcmp(data.data(), &open, sizeof(open.handshake)) == 0;
}

short AsioNetworkService::getFreeId() const
{
	for (int i = 1; i < 1024; i++) {
		if (activeConnections.find(i) == activeConnections.end()) {
			return i;
		}
	}
	throw Exception("Unable to find empty connection id");
}
