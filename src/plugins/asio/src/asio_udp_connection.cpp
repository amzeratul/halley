#include "asio_udp_connection.h"

#include "halley/net/connection/network_packet.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

using namespace Halley;

AsioUDPConnection::AsioUDPConnection(UDPSocket& socket, UDPEndpoint remote)
	: socket(socket)
	, remote(std::move(remote))
	, status(ConnectionStatus::Connecting)
	, connectionId(0)
{
}

void AsioUDPConnection::close()
{
	status = ConnectionStatus::Closing;
}

bool AsioUDPConnection::isSupported(TransmissionType type) const
{
	return type == TransmissionType::Unreliable;
}

void AsioUDPConnection::terminateConnection()
{
	if (status != ConnectionStatus::Closed) {
		status = ConnectionStatus::Closed;
	}
}

String AsioUDPConnection::getRemoteAddress() const
{
	const auto& remote = socket.remote_endpoint();
	return remote.address().to_string() + ":" + toString(static_cast<int32_t>(remote.port()));
}

void AsioUDPConnection::send(TransmissionType type, OutboundNetworkPacket packet)
{
    throw Exception("Not implemented", HalleyExceptions::Network);
}

bool AsioUDPConnection::receive(InboundNetworkPacket& packet)
{
    throw Exception("Not implemented", HalleyExceptions::Network);
}

bool AsioUDPConnection::matchesEndpoint(const UDPEndpoint& remoteEndpoint) const
{
	return remote == remoteEndpoint;
}

void AsioUDPConnection::setError(const std::string& cs)
{
	error = cs;
}

size_t AsioUDPConnection::getMaxUnreliablePacketSize() const
{
    return 1400;
}

void AsioUDPConnection::onConnect(short connId)
{
    if (status == ConnectionStatus::Connecting) {
        Logger::logDev("Connection established as id = " + toString(connId));
        connectionId = connId;
        status = ConnectionStatus::Connected;
    }
}

void AsioUDPConnection::sendUnreliablePacket(gsl::span<const gsl::byte> packet)
{
    if (status != ConnectionStatus::Connected && status != ConnectionStatus::Connecting) {
        Logger::logError("Attempting to send packet, but not in connected state", true);
        return;
    }

    auto buffer = asio::buffer(packet.data(), packet.size());
    asio::error_code err;

    size_t size = socket.send_to(buffer, remote, 0, err);

    if (err) {
        Logger::logError("Error sending packet: " + err.message());
        close();
    } else if (size != packet.size()) {
        Logger::logError("Error sending packet, size mismatch");
        close();
    }

    if (status == ConnectionStatus::Connected && packetListener != nullptr) {
        packetListener->onSend(packet);
    }
}

void AsioUDPConnection::setUnreliablePacketListener(IPacketListener* listener)
{
    packetListener = listener;
}

void AsioUDPConnection::receiveAll(
        UDPSocket &socket, HashMap<short, std::shared_ptr<AsioUDPConnection>> &connections,
        const std::function<void(UDPEndpoint &remote, gsl::span<gsl::byte> packet)> &unknownConnCallback)
{
    std::array<gsl::byte, 2048> cache = {};
    auto buffer = asio::buffer(cache.data(), cache.size());

    for (;;) {
        UDPEndpoint from;
        asio::error_code err;

        size_t size = socket.receive_from(buffer, from, 0, err);

        if (err) {
            if (err == asio::error::would_block) {
                break;
            } else {
                if (err == asio::error::connection_reset) {
                    Logger::logWarning("Connection reset by peer");
                } else {
                    Logger::logError("Error receiving packet: " + err.message());
                }
                for (auto& conn: connections) {
                    conn.second->close();
                }
                break;
            }
        }

        if (size == 0) {
            break;
        }

        bool foundEndpoint = false;
        for (const auto& pair : connections) {
            const auto& conn = pair.second;
            if (conn->matchesEndpoint(from) && conn->getStatus() == ConnectionStatus::Connected) {
                if (conn->packetListener != nullptr) {
                    conn->packetListener->onReceive(gsl::span(cache.data(), size));
                } else {
                    Logger::logError("No packet listener registered, packet will be lost", true);
                }
                foundEndpoint = true;
                break;
            }
        }

        if (!foundEndpoint) {
            unknownConnCallback(from, gsl::span(cache.data(), size));
        }
    }
}
