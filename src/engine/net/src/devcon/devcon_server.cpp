#include "devcon/devcon_server.h"
#include "connection/network_service.h"
#include "halley/support/logger.h"
#include <memory>
#include "connection/message_queue_tcp.h"
using namespace Halley;

DevConServerConnection::DevConServerConnection(std::shared_ptr<IConnection> conn)
	: connection(conn)
	, queue(std::make_shared<MessageQueueTCP>(connection))
{
	DevCon::setupMessageQueue(*queue);
}

void DevConServerConnection::update()
{
	for (auto& m: queue->receiveAll()) {
		auto& msg = dynamic_cast<DevCon::DevConMessage&>(*m);
		switch (msg.getMessageType()) {
		case DevCon::MessageType::Log:
			onReceiveLogMsg(dynamic_cast<DevCon::LogMsg&>(msg));
			break;

		case DevCon::MessageType::ReloadAssets:
			// TODO;

		default:
			break;
		}
	}
}

void DevConServerConnection::onReceiveLogMsg(const DevCon::LogMsg& msg)
{
	Logger::log(msg.getLevel(), "[REMOTE] " + msg.getMessage());
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
