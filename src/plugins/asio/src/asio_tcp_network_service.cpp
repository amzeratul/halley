#include "asio_tcp_network_service.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"
using namespace Halley;

AsioTCPNetworkService::AsioTCPNetworkService(int port, IPVersion version)
	: localEndpoint(version == IPVersion::IPv4 ? asio::ip::tcp::v4() : asio::ip::tcp::v6(), static_cast<unsigned short>(port))
	, acceptor(service, localEndpoint)
{
	Expects(port == 0 || port > 1024);
	Expects(port < 65536);
}

void AsioTCPNetworkService::update()
{
	service.poll();

	activeConnections.erase(std::remove_if(activeConnections.begin(), activeConnections.end(), [] (const std::shared_ptr<IConnection>& conn) { return conn->getStatus() == ConnectionStatus::Closed; }), activeConnections.end());
}

void AsioTCPNetworkService::setAcceptingConnections(bool accepting)
{
	if (acceptingConnection != accepting) {
		acceptingConnection = accepting;

		if (accepting) {
			acceptingSocket = TCPSocket(service);
			acceptor.async_accept(acceptingSocket.get(), [this] (const boost::system::error_code& ec) {
				if (ec) {
					Logger::logError("Error accepting connection: " + ec.message());
				} else {
					if (acceptingConnection) {
						onConnectionAccepted();
					}
				}
			});
		}
	}
}

std::shared_ptr<IConnection> AsioTCPNetworkService::tryAcceptConnection()
{
	if (!pendingConnections.empty()) {
		auto res = pendingConnections.front();
		pendingConnections.erase(pendingConnections.begin());
		return res;
	} else {
		return {};
	}
}

std::shared_ptr<IConnection> AsioTCPNetworkService::connect(String address, int port)
{
	auto conn = std::make_shared<AsioTCPConnection>(service, address, port);
	activeConnections.push_back(conn);
	return conn;
}

void AsioTCPNetworkService::onConnectionAccepted()
{
	// Move socket into a new connection
	pendingConnections.push_back(std::make_shared<AsioTCPConnection>(service, std::move(acceptingSocket.get())));
	activeConnections.push_back(pendingConnections.back());
	acceptingSocket = {};

	// Accept more
	acceptingConnection = false;
	setAcceptingConnections(true);
}
