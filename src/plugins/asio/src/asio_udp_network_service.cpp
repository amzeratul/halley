#include "halley/data_structures/vector.h"
#include <list>
#include "halley/net/connection/network_packet.h"
#include "asio_udp_network_service.h"
#include "asio_udp_connection.h"
#include <iostream>
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



AsioUDPNetworkService::AsioUDPNetworkService(int port, IPVersion version)
	: localEndpoint(version == IPVersion::IPv4 ? asio::ip::udp::v4() : asio::ip::udp::v6(), static_cast<unsigned short>(port))
	, socket(service, localEndpoint)
{
	Expects(port == 0 || port > 1024);
	Expects(port < 65536);
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
}

std::shared_ptr<IConnection> AsioUDPNetworkService::connect(const String& address)
{
	const auto splitAddr = address.split(':');
	const auto addr = splitAddr.at(0);
	const auto port = splitAddr.at(1).toInteger();
	
	assert(port > 1024);
	assert(port < 65536);
	auto remoteAddr = asio::ip::address::from_string(addr.cppStr());
	auto remote = UDPEndpoint(remoteAddr, static_cast<unsigned short>(port)); 
	auto conn = std::make_shared<AsioUDPConnection>(socket, remote);
	activeConnections[0] = conn;

	// Handshake
	HandshakeOpen open;
	conn->send(IConnection::TransmissionType::Unreliable, OutboundNetworkPacket(gsl::as_bytes(gsl::span<HandshakeOpen>(&open, 1))));

	startListening({}); // Hmm, this might not be right

	return conn;
}

String AsioUDPNetworkService::startListening(AcceptCallback callback)
{
	acceptCallback = std::move(callback);
	if (!startedListening) {
		startedListening = true;
		receiveNext();
	}
	return "";
}

void AsioUDPNetworkService::stopListening()
{
	acceptCallback = {};
}

void AsioUDPNetworkService::receiveNext()
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

void AsioUDPNetworkService::receivePacket(gsl::span<gsl::byte> received, std::string* error)
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
		id = short(bytes[0] & 0x7F) | short(bytes[1]);
	} else {
		received = received.subspan(1);
		id = short(bytes[0]);
	}

	// No connection id, check if it's a connection request
	if (id == 0 && isValidConnectionRequest(received)) {
		auto a = UDPAcceptor(*this, remoteEndpoint);
		if (acceptCallback) {
			acceptCallback(a);
		}
		a.ensureChoiceMade();
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

bool AsioUDPNetworkService::isValidConnectionRequest(gsl::span<const gsl::byte> data)
{
	HandshakeOpen open;
	return acceptCallback && data.size() == sizeof(open) && memcmp(data.data(), &open, sizeof(open.handshake)) == 0;
}

short AsioUDPNetworkService::getFreeId() const
{
	for (int i = 1; i < 1024; i++) {
		if (activeConnections.find(i) == activeConnections.end()) {
			return static_cast<short>(i);
		}
	}
	throw Exception("Unable to find empty connection id", HalleyExceptions::NetworkPlugin);
}

std::shared_ptr<AsioUDPConnection> AsioUDPNetworkService::acceptConnection(UDPEndpoint endPoint)
{
	auto conn = std::make_shared<AsioUDPConnection>(socket, endPoint);
	short id = getFreeId();
	conn->open(id);

	activeConnections[id] = conn;
	return conn;
}

void AsioUDPNetworkService::rejectConnection()
{
	
}

AsioUDPNetworkService::UDPAcceptor::UDPAcceptor(AsioUDPNetworkService& service, UDPEndpoint endPoint)
	: service(service)
	, endPoint(endPoint)
{}

std::shared_ptr<IConnection> AsioUDPNetworkService::UDPAcceptor::doAccept()
{
	return service.acceptConnection(endPoint);
}

void AsioUDPNetworkService::UDPAcceptor::doReject()
{
	return service.rejectConnection();
}
