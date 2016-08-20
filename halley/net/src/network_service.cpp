#include <vector>
#include <list>
#include <boost/asio.hpp>
#include "network_service.h"
#include "network_packet.h"
#include "udp_connection.h"
#include <iostream>
#include <unordered_map>
#include <halley/support/exception.h>

using namespace Halley;
namespace asio = boost::asio;

namespace Halley
{
	class NetworkServicePImpl
	{
	public:
		NetworkServicePImpl(int port, IPVersion version)
			: localEndpoint(version == IPVersion::IPv4 ? asio::ip::udp::v4() : asio::ip::udp::v6(), port)
			, socket(service, localEndpoint)
		{
			Expects(port == 0 || port > 1024);
			Expects(port < 65536);
		}

		asio::io_service service;
		UDPEndpoint localEndpoint;
		UDPEndpoint remoteEndpoint;
		asio::ip::udp::socket socket;
		std::list<UDPEndpoint> pendingIncomingConnections;
		std::unordered_map<short, std::shared_ptr<UDPConnection>> activeConnections;

		std::array<gsl::byte, 2048> receiveBuffer;
	};
}

struct HandshakeOpen
{
	HandshakeOpen()
		: handshake("halley_open")
	{
	}

	const char handshake[12];
	// TODO: public-key encrypted temporary key
};





NetworkService::NetworkService(int port, IPVersion version)
	: pimpl(std::make_unique<NetworkServicePImpl>(port, version))
{
}

NetworkService::~NetworkService()
{
	for (auto& conn : pimpl->activeConnections) {
		try {
			conn.second->terminateConnection();
		} catch (...) {
			std::cout << "Error terminating connection on ~NetworkService()" << std::endl;
		}
	}
	try {
		pimpl->service.poll();
		pimpl->socket.shutdown(UDPSocket::shutdown_both);
	} catch (...) {
		std::cout << "Error polling service on ~NetworkService()" << std::endl;
	}
}

void NetworkService::update()
{
	// Remove closed connections
	std::vector<short> toErase;
	auto& active = pimpl->activeConnections;
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
	pimpl->service.poll();
}

void NetworkService::setAcceptingConnections(bool accepting)
{
	acceptingConnections = accepting;
	if (accepting) {
		startListening();
	} else {
		pimpl->pendingIncomingConnections.clear();
	}
}

std::shared_ptr<IConnection> NetworkService::tryAcceptConnection()
{
	auto& pending = pimpl->pendingIncomingConnections;

	if (pending.empty()) {
		return nullptr;
	} else {
		auto conn = std::make_shared<UDPConnection>(pimpl->socket, pending.front());
		short id = getFreeId();
		conn->open(id);

		pimpl->activeConnections[id] = conn;
		pending.pop_front();
		return conn;
	}
}

std::shared_ptr<IConnection> NetworkService::connect(String addr, int port)
{
	Expects(port > 1024);
	Expects(port < 65536);
	auto remoteAddr = asio::ip::address::from_string(addr.cppStr());
	auto remote = UDPEndpoint(remoteAddr, port); 
	auto conn = std::make_shared<UDPConnection>(pimpl->socket, remote);
	pimpl->activeConnections[0] = conn;

	// Handshake
	HandshakeOpen open;
	conn->send(OutboundNetworkPacket(gsl::as_bytes(gsl::span<HandshakeOpen>(&open, 1))));

	startListening();

	return conn;
}

void NetworkService::startListening()
{
	if (!startedListening) {
		startedListening = true;
		receiveNext();
	}
}

void NetworkService::receiveNext()
{
	auto buffer = asio::buffer(pimpl->receiveBuffer);
	pimpl->socket.async_receive_from(buffer, pimpl->remoteEndpoint, [this] (const boost::system::error_code& error, size_t size)
	{
		try {
			Expects(size <= pimpl->receiveBuffer.size());

			std::string errorMsg;
			std::string* errorMsgPtr = nullptr;
			if (error) {
				errorMsg = error.message();
				errorMsgPtr = &errorMsg;
			}

			receivePacket(gsl::span<gsl::byte>(pimpl->receiveBuffer.data(), size), errorMsgPtr);
		} catch (...) {
			std::cout << "Exception while receiving a packet." << std::endl;
		}

		receiveNext();
	});
}

void NetworkService::receivePacket(gsl::span<gsl::byte> received, std::string* error)
{
	if (error) {
		std::cout << "Error receiving packet: " << (*error) << std::endl;
		// Find the owner of this remote endpoint
		for (auto& conn : pimpl->activeConnections) {
			if (conn.second->matchesEndpoint(pimpl->remoteEndpoint)) {
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
		auto& pending = pimpl->pendingIncomingConnections;
		if (std::find(pending.begin(), pending.end(), pimpl->remoteEndpoint) == pending.end()) {
			pending.push_back(pimpl->remoteEndpoint);
		}
		// Pending connection is valid
		return;
	}

	// Find the owner of this remote endpoint
	auto conn = pimpl->activeConnections.find(id);
	if (conn == pimpl->activeConnections.end()) {
		// Connection doesn't exist, but check the pending slot
		conn = pimpl->activeConnections.find(0);
		if (conn == pimpl->activeConnections.end()) {
			// Nope, give up
			return;
		}
	}

	// Validate that this connection is who it claims to be
	if (conn->second->matchesEndpoint(pimpl->remoteEndpoint)) {
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
						pimpl->activeConnections[newId] = connection;
						pimpl->activeConnections.erase(conn);
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

bool NetworkService::isValidConnectionRequest(gsl::span<const gsl::byte> data)
{
	HandshakeOpen open;
	return acceptingConnections && data.size() == sizeof(open) && memcmp(data.data(), &open, sizeof(open.handshake)) == 0;
}

short NetworkService::getFreeId() const
{
	for (int i = 1; i < 1024; i++) {
		if (pimpl->activeConnections.find(i) == pimpl->activeConnections.end()) {
			return i;
		}
	}
	throw Exception("Unable to find empty connection id");
}
