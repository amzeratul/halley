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
		NetworkServicePImpl(int port)
			: localEndpoint(asio::ip::udp::v4(), port)
			, socket(service, localEndpoint)
		{
			assert(port > 1024);
			assert(port < 65536);
		}

		asio::io_service service;
		UDPEndpoint localEndpoint;
		UDPEndpoint remoteEndpoint;
		asio::ip::udp::socket socket;
		std::list<UDPEndpoint> pendingIncomingConnections;

		std::array<char, 2048> receiveBuffer;
	};
}

NetworkService::NetworkService(int port)
	: pimpl(std::make_unique<NetworkServicePImpl>(port))
{
}

NetworkService::~NetworkService()
{
}

void NetworkService::update()
{
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

IConnection* NetworkService::tryAcceptConnection()
{
	auto& pending = pimpl->pendingIncomingConnections;

	if (pending.empty()) {
		return nullptr;
	} else {
		activeConnections.push_back(std::make_unique<UDPConnection>(pimpl->socket, pending.front()));
		pending.pop_front();
		return activeConnections.back().get();
	}
}

IConnection* NetworkService::connect(String addr, int port)
{
	assert(port > 1024);
	assert(port < 65536);
	auto remoteAddr = asio::ip::address::from_string(addr.cppStr());
	auto remote = UDPEndpoint(remoteAddr, port); 
	activeConnections.push_back(std::make_unique<UDPConnection>(pimpl->socket, remote));
	auto conn = activeConnections.back().get();

	// Send handshake
	// TODO
	conn->send(NetworkPacket());

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
	std::cout << "receiveNext()" << std::endl;
	auto buffer = asio::buffer(pimpl->receiveBuffer);
	pimpl->socket.async_receive_from(buffer, pimpl->remoteEndpoint, [this] (const boost::system::error_code& error, size_t size)
	{
		std::cout << "Received packet with " << size << " bytes." << std::endl;

		// Find the owner of this remote endpoint
		

		if (error) {
			// TODO
		} else {
			// TODO
		}

		receiveNext();
	});
}
