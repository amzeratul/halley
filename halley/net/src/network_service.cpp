#include "network_service.h"
#include <boost/asio.hpp>
#include "udp_connection.h"
#include <iostream>
#include <network_packet.h>

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
			assert(port == 0 || port > 1024);
			assert(port < 65536);
		}

		asio::io_service service;
		UDPEndpoint localEndpoint;
		UDPEndpoint remoteEndpoint;
		asio::ip::udp::socket socket;
		std::list<UDPEndpoint> pendingIncomingConnections;
		std::vector<std::shared_ptr<UDPConnection>> activeConnections;

		std::array<char, 2048> receiveBuffer;
	};
}

NetworkService::NetworkService(int port, IPVersion version)
	: pimpl(std::make_unique<NetworkServicePImpl>(port, version))
{
}

NetworkService::~NetworkService()
{
	for (auto& conn : pimpl->activeConnections) {
		conn->terminateConnection();
	}
	pimpl->service.poll();
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
		pimpl->activeConnections.push_back(std::make_shared<UDPConnection>(pimpl->socket, pending.front()));
		pending.pop_front();
		return pimpl->activeConnections.back();
	}
}

std::shared_ptr<IConnection> NetworkService::connect(String addr, int port)
{
	assert(port > 1024);
	assert(port < 65536);
	auto remoteAddr = asio::ip::address::from_string(addr.cppStr());
	auto remote = UDPEndpoint(remoteAddr, port); 
	pimpl->activeConnections.push_back(std::make_shared<UDPConnection>(pimpl->socket, remote));
	auto& conn = pimpl->activeConnections.back();

	// Handshake
	conn->send(NetworkPacket("halley_open_connection", 23));

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
		// Find the owner of this remote endpoint
		UDPConnection* connection = nullptr;
		for (auto& conn : pimpl->activeConnections) {
			if (conn->matchesEndpoint(pimpl->remoteEndpoint)) {
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
		} else {
			if (connection) {
				connection->onReceive(pimpl->receiveBuffer.data(), size);
			} else {
				onNewConnectionRequest(pimpl->receiveBuffer.data(), size, pimpl->remoteEndpoint);
			}
		}

		receiveNext();
	});
}

void NetworkService::onNewConnectionRequest(char* data, size_t size, const UDPEndpoint& remoteEndpoint)
{
	if (acceptingConnections && size == 23 && memcmp(data, "halley_open_connection", 23) == 0) {
		pimpl->pendingIncomingConnections.push_back(remoteEndpoint);
	}
}
