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
	queue->sendAll();

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
}

void DevConServerConnection::registerInterest(const String& id, const ConfigNode& params, uint32_t handle)
{
	// TODO
}

void DevConServerConnection::unregisterInterest(uint32_t handle)
{
	// TODO
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
		initConnection(*connections.back());
	});
}

void DevConServer::update(Time t)
{
	service->update(t);

	for (const auto& c: connections) {
		c->update(t);
	}
}

void DevConServer::reloadAssets(gsl::span<const String> ids)
{
	for (const auto& c: connections) {
		c->reloadAssets(ids);
	}
}

DevConServer::InterestHandle DevConServer::registerInterest(String id, ConfigNode params, InterestCallback callback)
{
	const InterestHandle handle = interestId++;

	for (const auto& c: connections) {
		c->registerInterest(id, params, handle);
	}

	interest[handle] = Interest{ std::move(id), std::move(params), std::move(callback) };

	return handle;
}

void DevConServer::unregisterInterest(InterestHandle handle)
{
	for (const auto& c: connections) {
		c->unregisterInterest(handle);
	}
	interest.erase(handle);
}

void DevConServer::initConnection(DevConServerConnection& conn)
{
	for (const auto& [handle, val]: interest) {
		conn.registerInterest(val.id, val.config, handle);
	}
}
