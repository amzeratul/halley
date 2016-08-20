#include <vector>
#include <list>
#include <boost/asio.hpp>
#include "network_service.h"
#include "network_packet.h"
#include "udp_connection.h"
#include <iostream>

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
		std::vector<std::shared_ptr<UDPConnection>> activeConnections;

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
			conn->terminateConnection();
		} catch (...) {
			std::cout << "Error terminating connection on ~NetworkService()" << std::endl;
		}
	}
	try {
		pimpl->service.poll();
	} catch (...) {
		std::cout << "Error polling service on ~NetworkService()" << std::endl;
	}
}

void NetworkService::update()
{
	// Remove closed connections
	auto& active = pimpl->activeConnections;
	size_t n = active.size();
	for (size_t i = 0; i < n; i++) {
		auto& c = active[i];
		if (c->getStatus() == ConnectionStatus::CLOSING) {
			c->terminateConnection();
			active.erase(active.begin() + i);
			--i;
			--n;
		}
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

		pimpl->activeConnections.push_back(conn);
		pending.pop_front();
		return pimpl->activeConnections.back();
	}
}

std::shared_ptr<IConnection> NetworkService::connect(String addr, int port)
{
	Expects(port > 1024);
	Expects(port < 65536);
	auto remoteAddr = asio::ip::address::from_string(addr.cppStr());
	auto remote = UDPEndpoint(remoteAddr, port); 
	pimpl->activeConnections.push_back(std::make_shared<UDPConnection>(pimpl->socket, remote));
	auto& conn = pimpl->activeConnections.back();

	// Handshake
	HandshakeOpen open;
	conn->send(OutboundNetworkPacket(gsl::span<HandshakeOpen>(&open, sizeof(HandshakeOpen))));

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
			auto received = gsl::span<gsl::byte>(pimpl->receiveBuffer.data(), size);
			//std::cout << "Received " << size << " bytes\n";
			UDPConnection* connection = nullptr;

			short id = -1; // TODO
			if (size >= 1) {
				// Read connection id
				received = received.subspan(1);
			}

			// Find the owner of this remote endpoint
			for (auto& conn : pimpl->activeConnections) {
				if (conn->matchesEndpoint(id, pimpl->remoteEndpoint)) {
					connection = conn.get();
					break;
				}
			}

			if (error) {
				// Close the connection if there was an error
				if (connection) {
					connection->setError(error.message());
					connection->close();
				}
			}
			else {
				if (connection) {
					try {
						connection->onReceive(received);
					} catch (std::exception& e) {
						connection->setError(e.what());
						connection->close();
					} catch (...) {
						connection->setError("Unknown error receiving packet.");
						connection->close();
					}
				} else {
					if (isValidConnectionRequest(received)) {
						auto& pending = pimpl->pendingIncomingConnections;
						if (std::find(pending.begin(), pending.end(), pimpl->remoteEndpoint) == pending.end()) {
							pending.push_back(pimpl->remoteEndpoint);
						}
					}
				}
			}
		} catch (...) {
			std::cout << "Exception while receiving a packet." << std::endl;
		}

		receiveNext();
	});
}

bool NetworkService::isValidConnectionRequest(gsl::span<const gsl::byte> data)
{
	HandshakeOpen open;
	return acceptingConnections && data.size() == sizeof(open) && memcmp(data.data(), &open, sizeof(open.handshake)) == 0;
}

short NetworkService::getFreeId() const
{
	// TODO
	return -1;
}
