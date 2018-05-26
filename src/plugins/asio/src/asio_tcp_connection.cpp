#include "asio_tcp_connection.h"
#include "halley/support/logger.h"
#include "halley/text/halleystring.h"
#include "halley/text/string_converter.h"
#include "halley/net/connection/network_packet.h"
using namespace Halley;

AsioTCPConnection::AsioTCPConnection(asio::io_service& service, TCPSocket socket)
	: service(service)
	, socket(std::move(socket))
	, status(ConnectionStatus::Connected)
{}

AsioTCPConnection::AsioTCPConnection(asio::io_service& service, String host, int port)
	: service(service)
	, socket(service)
	, status(ConnectionStatus::Connecting)
{
	resolver = std::make_unique<asio::ip::tcp::resolver>(service);
	resolver->async_resolve(host.cppStr(), toString(port).cppStr(), [=] (const boost::system::error_code& ec, asio::ip::tcp::resolver::results_type result)
	{
		if (ec) {
			Logger::logError("Error trying to connect to " + host + ":" + toString(port) + ": " + ec.message());
		} else {
			for (auto& r: result) {
				boost::system::error_code ec2;
				socket.connect(r, ec2);
				if (!ec2) {
					Logger::logInfo("Connected");
					status = ConnectionStatus::Connected;
					break;
				}
			}

			if (status != ConnectionStatus::Connected) {
				Logger::logError("Error trying to connect to " + host + ":" + toString(port) + ". None of the endpoints were suitable.");
				status = ConnectionStatus::Closing;
			}
		}
	});
}

AsioTCPConnection::~AsioTCPConnection()
{
	AsioTCPConnection::close();
}

void AsioTCPConnection::update()
{
	if (status == ConnectionStatus::Closing) {
		close();
	}
	if (status == ConnectionStatus::Connected) {
		tryReceive();
		if (!socket.is_open()) {
			close();
		}
	}
}

void AsioTCPConnection::close()
{
	resolver.reset();
	if (status != ConnectionStatus::Closed) {
		Logger::logInfo("Disconnected");
		try {
			socket.close();
		} catch (std::exception& e) {
			Logger::logException(e);
		} catch (...) {
			Logger::logError(String("Unknown error closing TCP connection"));
		}
		status = ConnectionStatus::Closed;
	}
}

ConnectionStatus AsioTCPConnection::getStatus() const
{
	return status;
}

void AsioTCPConnection::send(OutboundNetworkPacket&& packet)
{
	packet.addHeader(uint32_t(packet.getSize()));

	auto bytes = packet.getBytes();
	auto bs = Bytes(bytes.size_bytes());
	memcpy(bs.data(), bytes.data(), bytes.size());
	sendQueue.emplace_back(std::move(bs));

	if (sendQueue.size() == 1) {
		sendMore();
	}
}

bool AsioTCPConnection::receive(InboundNetworkPacket& packet)
{
	tryReceive();

	if (receiveQueue.size() >= sizeof(uint32_t)) {
		const uint32_t size = *reinterpret_cast<uint32_t*>(receiveQueue.data());
		if (size > 128 * 1024 * 1024) {
			Logger::logError("Invalid packet size.");
			close();
		}

		const auto packetSize = sizeof(uint32_t) + size;
		if (receiveQueue.size() >= packetSize) {
			packet = InboundNetworkPacket(gsl::as_bytes(gsl::span<Byte>(receiveQueue.data(), packetSize)));
			uint32_t size2;
			packet.extractHeader(size2);
			receiveQueue.erase(receiveQueue.begin(), receiveQueue.begin() + packetSize);
			return true;
		}
	}
	
	return false;
}

void AsioTCPConnection::sendMore()
{
	if (!sendQueue.empty()) {
		sendBuffer(sendQueue.front());
	}
}

void AsioTCPConnection::sendBuffer(Bytes& toSend)
{
	socket.async_write_some(asio::buffer(toSend.data(), toSend.size()), [=] (const boost::system::error_code& ec, size_t bytesWritten)
	{
		if (ec) {
			Logger::logError("Error sending data on TCP socket: " + ec.message());
			close();
		} else {
			auto& front = sendQueue.front();
			if (bytesWritten == front.size()) {
				sendQueue.pop_front();
			} else {
				front.erase(front.begin(), front.begin() + bytesWritten);
			}

			sendMore();
		}
	});	
}

void AsioTCPConnection::tryReceive()
{
	if (receiveBuffer.size() < 1024) {
		receiveBuffer.resize(1024);
	}

	socket.async_read_some(asio::mutable_buffer(receiveBuffer.data(), receiveBuffer.size()), [=] (const boost::system::error_code& ec, size_t bytesReceived)
	{
		if (ec) {
			Logger::logError("Error reading data on TCP socket: " + ec.message());
			close();
		} else {
			size_t curPos = receiveQueue.size();
			receiveQueue.resize(curPos + bytesReceived);
			memcpy(receiveQueue.data() + curPos, receiveBuffer.data(), bytesReceived);

			tryReceive();
		}
	});
}
