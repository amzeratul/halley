#include "udp_connection.h"
#include <network_packet.h>
#include <iostream>

using namespace Halley;

UDPConnection::UDPConnection(UDPSocket& socket, UDPEndpoint remote)
	: socket(socket)
	, remote(remote)
{
	
}

void UDPConnection::close()
{
	
}

void UDPConnection::send(NetworkPacket&& packet)
{
	bool needsSend = pendingSend.empty();
	pendingSend.emplace_back(std::move(packet));
	if (needsSend) {
		sendNext();
	}
}

bool UDPConnection::receive(NetworkPacket& packet)
{
	return false;
}

void UDPConnection::sendNext()
{
	auto& packet = pendingSend.front();
	size_t size = packet.copyTo(sendBuffer.data(), sendBuffer.size());
	pendingSend.pop_front();

	socket.async_send_to(boost::asio::buffer(sendBuffer, size), remote, [this] (const boost::system::error_code& error, std::size_t size)
	{
		if (error) {
			std::cout << "Error sending packet: " << error.message() << std::endl;
		} else {
			std::cout << "Sent " << size << " bytes." << std::endl;
		}

		if (!pendingSend.empty()) {
			sendNext();
		}
	});
}
