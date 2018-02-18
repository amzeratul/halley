#include "devcon/devcon_server.h"
#include "connection/network_service.h"
#include "halley/support/logger.h"
using namespace Halley;

DevConServerConnection::DevConServerConnection(std::shared_ptr<IConnection> conn)
	: connection(std::make_shared<ReliableConnection>(conn))
	, queue(std::make_shared<MessageQueue>(connection))
{
	queue->setChannel(0, ChannelSettings(true, true));
}

void DevConServerConnection::update()
{
	for (auto& msg: queue->receiveAll()) {
		// TODO
	}
}

DevConServer::DevConServer(std::unique_ptr<NetworkService> s, int port)
	: service(std::move(s))
{
	service->setAcceptingConnections(true);
}

void DevConServer::update()
{
	service->update();

	auto newCon = service->tryAcceptConnection();
	if (newCon) {
		Logger::logInfo("New incoming DevCon connection.");
		connections.push_back(std::make_shared<DevConServerConnection>(newCon));
	}

	for (auto& c: connections) {
		c->update();
	}
}
