#include "asio_tcp_connection.h"
#include "halley/support/logger.h"
#include "halley/text/halleystring.h"
#include "halley/text/string_converter.h"
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
	if (status == ConnectionStatus::Connected && !socket.is_open()) {
		close();
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
	// TODO
}

bool AsioTCPConnection::receive(InboundNetworkPacket& packet)
{
	// TODO
	return false;
}
