#include "asio_tcp_network_service.h"
#include "halley/support/logger.h"
#include "halley/text/string_converter.h"
using namespace Halley;

AsioTCPNetworkService::AsioTCPNetworkService(int port, IPVersion version)
	: work(service)
	, localEndpoint(version == IPVersion::IPv4 ? asio::ip::tcp::v4() : asio::ip::tcp::v6(), static_cast<unsigned short>(port))
	, acceptor(service, localEndpoint)
{
	Expects(port == 0 || port > 1024);
	Expects(port < 65536);
}

void AsioTCPNetworkService::update(Time t)
{
	NetworkServiceWithStats::update(t);
	
	bool needsPoll;
	do {
		needsPoll = false;
		for (auto& conn: activeConnections) {
			needsPoll |= conn->needsPolling();
			conn->update();
		}
		service.poll();
	} while (needsPoll);

	activeConnections.erase(std::remove_if(activeConnections.begin(), activeConnections.end(), [] (const std::shared_ptr<IConnection>& conn) { return conn->getStatus() == ConnectionStatus::Closed; }), activeConnections.end());
}

String AsioTCPNetworkService::startListening(AcceptCallback callback)
{
	acceptCallback = std::move(callback);
	
	doStartListening();

	return "";
}

void AsioTCPNetworkService::doStartListening()
{
	acceptor.cancel();
	acceptingSocket = TCPSocket(service);
	acceptor.async_accept(acceptingSocket.value(), [this] (const boost::system::error_code& ec) {
		if (ec) {
			Logger::logError("Error accepting connection: " + ec.message());
		} else {
			auto a = TCPAcceptor(*this);
			if (acceptCallback) {
				acceptCallback(a);
			}
			a.ensureChoiceMade();
		}
	});
}

void AsioTCPNetworkService::stopListening()
{
	acceptCallback = {};
	acceptor.cancel();
}

std::shared_ptr<IConnection> AsioTCPNetworkService::connect(const String& address)
{
	const auto splitAddr = address.split(':');
	auto conn = std::make_shared<AsioTCPConnection>(service, splitAddr.at(0), splitAddr.at(1).toInteger());
	activeConnections.push_back(conn);
	return conn;
}

AsioTCPNetworkService::TCPAcceptor::TCPAcceptor(AsioTCPNetworkService& service)
	: service(service)
{}

std::shared_ptr<IConnection> AsioTCPNetworkService::TCPAcceptor::doAccept()
{
	return service.acceptConnection();
}

void AsioTCPNetworkService::TCPAcceptor::doReject()
{
	service.rejectConnection();
}

std::shared_ptr<AsioTCPConnection> AsioTCPNetworkService::acceptConnection()
{
	auto conn = std::make_shared<AsioTCPConnection>(service, std::move(acceptingSocket.value()));
	activeConnections.push_back(conn);
	doStartListening();
	return conn;
}

void AsioTCPNetworkService::rejectConnection()
{
	doStartListening();
}

