#include "devcon/devcon_server.h"
#include "halley/net/connection/network_service.h"
#include "halley/support/logger.h"
#include <memory>
#include "halley/net/connection/message_queue_tcp.h"
#include "halley/net/connection/iconnection.h"
#include "halley/net/connection/message_queue.h"
#include "devcon/devcon_messages.h"

using namespace Halley;

DevConServerConnection::DevConServerConnection(std::shared_ptr<IConnection> conn)
	: connection(conn)
	, queue(std::make_shared<MessageQueueTCP>(connection))
{
	DevCon::setupMessageQueue(*queue);
}

void DevConServerConnection::update(Time t)
{
	for (auto& m: queue->receiveMessages()) {
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

void DevConServerConnection::reloadAssets(gsl::span<const String> ids)
{
	queue->enqueue(std::make_unique<DevCon::ReloadAssetsMsg>(ids), 0);
	queue->sendAll();
}

void DevConServerConnection::onReceiveLogMsg(const DevCon::LogMsg& msg)
{
	Logger::log(msg.getLevel(), "[REMOTE] " + msg.getMessage());
}

DevConServer::DevConServer(std::unique_ptr<NetworkService> s, int port)
	: service(std::move(s))
{
	service->startListening([=] (NetworkService::Acceptor& a)
	{
		Logger::logInfo("New incoming DevCon connection.");
		connections.push_back(std::make_shared<DevConServerConnection>(a.accept()));
	});
}

void DevConServer::update(Time t)
{
	service->update(t);

	for (auto& c: connections) {
		c->update(t);
	}
}

void DevConServer::reloadAssets(gsl::span<const String> ids)
{
	for (auto& c: connections) {
		c->reloadAssets(ids);
	}
}
