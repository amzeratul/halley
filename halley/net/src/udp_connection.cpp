#include "udp_connection.h"
#include <network_packet.h>
#include <iostream>

using namespace Halley;

UDPConnection::UDPConnection(UDPSocket& socket, UDPEndpoint remote)
	: socket(socket)
	, remote(remote)
	, status(ConnectionStatus::OPEN)
{
}

void UDPConnection::close()
{
	onClose();
	status = ConnectionStatus::CLOSING;
}

void UDPConnection::terminateConnection()
{
	onClose();
	status = ConnectionStatus::CLOSED;
}

void UDPConnection::onClose()
{
	if (status == ConnectionStatus::OPEN) {
		// TODO: send close connection message
	}
}

void UDPConnection::send(NetworkPacket&& packet)
{
	if (status == ConnectionStatus::OPEN) {
		bool needsSend = pendingSend.empty();
		pendingSend.emplace_back(std::move(packet));
		if (needsSend) {
			sendNext();
		}
	}
}

bool UDPConnection::receive(NetworkPacket& packet)
{
	if (pendingReceive.empty()) {
		return false;
	} else {
		packet = std::move(pendingReceive.front());
		pendingReceive.pop_front();
		return true;
	}
}

bool UDPConnection::matchesEndpoint(const UDPEndpoint& remoteEndpoint) const
{
	return remote == remoteEndpoint;
}

void UDPConnection::onReceive(gsl::span<const gsl::byte> data)
{
	Expects(data.size() <= 1500);
	if (data.size() <= 1500) {
		pendingReceive.push_back(NetworkPacket(data));
	}
}

void UDPConnection::setError(const std::string& cs)
{
	error = cs;
}

void UDPConnection::sendNext()
{
	if (pendingSend.empty()) {
		return;
	}

	auto& packet = pendingSend.front();
	size_t size = packet.copyTo(sendBuffer);
	pendingSend.pop_front();

	socket.async_send_to(boost::asio::buffer(sendBuffer, size), remote, [this] (const boost::system::error_code& error, std::size_t size)
	{
		if (error) {
			std::cout << "Error sending packet: " << error.message() << std::endl;
			close();
		} else if (!pendingSend.empty()) {
			sendNext();
		}
	});
}
