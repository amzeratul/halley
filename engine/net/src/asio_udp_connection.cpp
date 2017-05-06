#include "asio_udp_connection.h"
#include <network_packet.h>
#include <iostream>

using namespace Halley;



struct HandshakeAccept
{
	HandshakeAccept()
		: handshake("halley_accp")
	{
	}

	const char handshake[12];
	short id = -1;
	// TODO: encrypted session key
};



AsioUDPConnection::AsioUDPConnection(UDPSocket& socket, UDPEndpoint remote)
	: socket(socket)
	, remote(remote)
	, status(ConnectionStatus::CONNECTING)
	, connectionId(0)
{
}

void AsioUDPConnection::close()
{
	status = ConnectionStatus::CLOSING;
}

void AsioUDPConnection::terminateConnection()
{
	if (status != ConnectionStatus::CLOSED) {
		status = ConnectionStatus::CLOSED;
	}
}

void AsioUDPConnection::send(OutboundNetworkPacket&& packet)
{
	if (status == ConnectionStatus::OPEN || status == ConnectionStatus::CONNECTING) {
		// Insert header
		std::array<unsigned char, 2> id = { 0, 0 };
		size_t len = 0;
		if (connectionId >= 128) {
			id[0] = (connectionId >> 8) & 0x7F;
			id[1] = connectionId & 0xFF;
			len = 2;
		} else {
			id[0] = connectionId & 0x7F;
			len = 1;
		}
		packet.addHeader(gsl::as_bytes(gsl::span<unsigned char>(id).subspan(0, len)));

		bool needsSend = pendingSend.empty();
		pendingSend.emplace_back(std::move(packet));
		if (needsSend) {
			sendNext();
		}
	}
}

bool AsioUDPConnection::receive(InboundNetworkPacket& packet)
{
	if (pendingReceive.empty()) {
		return false;
	} else {
		packet = std::move(pendingReceive.front());
		pendingReceive.pop_front();
		return true;
	}
}

bool AsioUDPConnection::matchesEndpoint(const UDPEndpoint& remoteEndpoint) const
{
	return remote == remoteEndpoint;
}

void AsioUDPConnection::onReceive(gsl::span<const gsl::byte> data)
{
	Expects(data.size() <= 1500);

	if (status == ConnectionStatus::CONNECTING) {
		if (data.size_bytes() == sizeof(HandshakeAccept)) {
			HandshakeAccept accept;
			if (memcmp(data.data(), &accept, sizeof(accept.handshake)) == 0) {
				// Yep, accept handshake
				memcpy(&accept, data.data(), data.size_bytes());
				onOpen(accept.id);
			}
		}
	} else if (status == ConnectionStatus::OPEN) {
		if (data.size() <= 1500) {
			pendingReceive.push_back(InboundNetworkPacket(data));
		}
	}
}

void AsioUDPConnection::setError(const std::string& cs)
{
	error = cs;
}

void AsioUDPConnection::open(short id)
{
	if (status == ConnectionStatus::CONNECTING) {
		// Handshake
		HandshakeAccept accept;
		accept.id = id;
		send(OutboundNetworkPacket(gsl::as_bytes(gsl::span<HandshakeAccept>(&accept, 1))));

		onOpen(id);
	}
}

void AsioUDPConnection::onOpen(short id)
{
	std::cout << "Connection open on id = " << id << std::endl;
	connectionId = id;
	status = ConnectionStatus::OPEN;
}

void AsioUDPConnection::sendNext()
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
