#include "devcon/devcon_server.h"
#include "halley/net/connection/network_service.h"
#include "halley/support/logger.h"
#include <memory>
#include "halley/net/connection/message_queue_tcp.h"
#include "halley/net/connection/iconnection.h"
#include "halley/net/connection/message_queue.h"
#include "devcon/devcon_messages.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

DevConServerConnection::DevConServerConnection(DevConServer& parent, size_t id, std::shared_ptr<IConnection> conn)
	: parent(parent)
	, id(id)
	, connection(conn)
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

		case DevCon::MessageType::NotifyInterest:
			onReceiveNotifyInterestMsg(dynamic_cast<DevCon::NotifyInterestMsg&>(msg));

		default:
			break;
		}
	}
}

bool DevConServerConnection::isAlive() const
{
	return connection->getStatus() != ConnectionStatus::Closed && connection->getStatus() != ConnectionStatus::Closing;
}

size_t DevConServerConnection::getId() const
{
	return id;
}

void DevConServerConnection::reloadAssets(gsl::span<const String> ids)
{
	queue->enqueue(std::make_unique<DevCon::ReloadAssetsMsg>(ids), 0);
}

void DevConServerConnection::registerInterest(const String& id, const ConfigNode& params, uint32_t handle)
{
	queue->enqueue(std::make_unique<DevCon::RegisterInterestMsg>(id, ConfigNode(params), handle), 0);
}

void DevConServerConnection::unregisterInterest(uint32_t handle)
{
	queue->enqueue(std::make_unique<DevCon::UnregisterInterestMsg>(handle), 0);
}



void DevConServerConnection::onReceiveLogMsg(DevCon::LogMsg& msg)
{
	Logger::log(msg.level, "[REMOTE] " + msg.msg);
}

void DevConServerConnection::onReceiveNotifyInterestMsg(DevCon::NotifyInterestMsg& msg)
{
	parent.onReceiveNotifyInterestMsg(*this, msg);
}

DevConServer::DevConServer(std::unique_ptr<NetworkService> s, int port)
	: service(std::move(s))
{
	service->startListening([=] (NetworkService::Acceptor& a)
	{
		Logger::logInfo("New incoming DevCon connection.");
		connections.push_back(std::make_shared<DevConServerConnection>(*this, connId++, a.accept()));
		initConnection(*connections.back());
	});
}

void DevConServer::update(Time t)
{
	service->update(t);

	for (const auto& c: connections) {
		c->update(t);
		if (!c->isAlive()) {
			terminateConnection(*c);
		}
	}

	std_ex::erase_if(connections, [=](const auto& c) { return !c->isAlive(); });
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

void DevConServer::onReceiveNotifyInterestMsg(const DevConServerConnection& connection, DevCon::NotifyInterestMsg& msg)
{
	const auto iter = interest.find(msg.handle);
	if (iter != interest.end()) {
		if (!std_ex::contains(iter->second.hadResult, connection.getId())) {
			iter->second.hadResult.push_back(connection.getId());
		}
		iter->second.callback(connection.getId(), std::move(msg.data));
	}
}

void DevConServer::initConnection(DevConServerConnection& conn)
{
	for (const auto& [handle, val]: interest) {
		conn.registerInterest(val.id, val.config, handle);
	}
}

void DevConServer::terminateConnection(DevConServerConnection& conn)
{
	for (auto& [k, v] : interest) {
		if (std_ex::contains(v.hadResult, conn.getId())) {
			std_ex::erase(v.hadResult, conn.getId());
			v.callback(conn.getId(), ConfigNode());
		}
	}
}
