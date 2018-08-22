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
					Logger::logDev("Connected to " + host + ":" + toString(port));
					status = ConnectionStatus::Connected;
					break;
				}
			}

			if (status != ConnectionStatus::Connected) {
				Logger::logError("Error trying to connect to " + host + ":" + toString(port) + ".");
				status = ConnectionStatus::Closing;
			}
		}
	});

	service.poll();
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
		{
			std::unique_lock<std::mutex> lock(mutex);
			needsPoll = false;
			tryReceive();
			trySend();
		}

		if (!socket.is_open()) {
			close();
		}
	}
}

void AsioTCPConnection::close()
{
	std::unique_lock<std::mutex> lock(mutex);
	resolver.reset();
	if (status != ConnectionStatus::Closed) {
		Logger::logInfo("Disconnected");
		try {
			if (!socket.is_open()) {
				socket.shutdown(TCPSocket::shutdown_both);
				socket.close();
			}
		} catch (std::exception& e) {
			Logger::logException(e);
		} catch (...) {
			Logger::logError(String("Unknown error closing TCP connection"));
		}
		needsPoll = false;
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

	std::unique_lock<std::mutex> lock(mutex);
	if (status == ConnectionStatus::Connected) {
		sendQueue.emplace_back(std::move(bs));

		if (sendQueue.size() == 1) {
			trySend();
		}
	}
}

bool AsioTCPConnection::receive(InboundNetworkPacket& packet)
{
	std::unique_lock<std::mutex> lock(mutex);
	if (receiveQueue.size() >= sizeof(uint32_t) && status == ConnectionStatus::Connected) {
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

bool AsioTCPConnection::needsPolling() const
{
	return needsPoll;
}

void AsioTCPConnection::trySend()
{
	if (!sendQueue.empty() && status == ConnectionStatus::Connected) {
		needsPoll = true;

		sendingQueue.emplace_back(std::move(sendQueue.front()));
		sendQueue.pop_front();
		auto& toSend = sendingQueue.back();

		socket.async_write_some(asio::buffer(toSend.data(), toSend.size()), [=] (const boost::system::error_code& ec, size_t bytesWritten)
		{
			if (ec) {
				Logger::logError("Error sending data on TCP socket: " + ec.message());
				close();
			} else {
				auto& front = sendingQueue.front();
				if (bytesWritten == front.size()) {
					sendingQueue.pop_front();
				} else {
					front.erase(front.begin(), front.begin() + bytesWritten);
				}

				trySend();
			}
		});
	}
}

void AsioTCPConnection::tryReceive()
{
	if (!reading && status == ConnectionStatus::Connected) {
		needsPoll = true;
		reading = true;

		if (receiveBuffer.size() < 4096) {
			receiveBuffer.resize(4096);
		}

		socket.async_read_some(asio::mutable_buffer(receiveBuffer.data(), receiveBuffer.size()), [=] (const boost::system::error_code& ec, size_t nBytesReceived)
		{
			reading = false;

			if (ec) {
				// retry on would_block and try_again errors, close connection on any other errors
				if (ec.value() == asio::error::would_block || ec.value() == asio::error::try_again) {
					tryReceive();
				} else {
					Logger::logError("Error reading data on TCP socket: " + ec.message());
					close();
				}
			} else {
				size_t curQueueSize = receiveQueue.size();
				receiveQueue.resize(curQueueSize + nBytesReceived);
				memcpy(receiveQueue.data() + curQueueSize, receiveBuffer.data(), nBytesReceived);

				if (nBytesReceived == 4096) {
					tryReceive();
				}
			}
		});
	}
}
