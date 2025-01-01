#include "halley/devcon/devcon_server.h"
#include "halley/net/connection/network_service.h"
#include "halley/support/logger.h"
#include <memory>
#include "halley/net/connection/message_queue_tcp.h"
#include "halley/net/connection/iconnection.h"
#include "halley/net/connection/message_queue.h"
#include "halley/devcon/devcon_messages.h"
#include "halley/game/scene_editor_interface.h"
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
			onReceiveMsg(dynamic_cast<DevCon::LogMsg&>(msg));
			break;

		case DevCon::MessageType::NotifyInterest:
			onReceiveMsg(dynamic_cast<DevCon::NotifyInterestMsg&>(msg));
			break;

		case DevCon::MessageType::SetClientData:
			onReceiveMsg(dynamic_cast<DevCon::SetClientDataMsg&>(msg));
			break;

		case DevCon::MessageType::RPCReply:
			onReceiveMsg(dynamic_cast<DevCon::RPCReplyMsg&>(msg));

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

void DevConServerConnection::reloadAssets(Vector<String> assetIds, Vector<String> packIds)
{
	queue->enqueue(std::make_unique<DevCon::ReloadAssetsMsg>(std::move(assetIds), std::move(packIds)), 0);
}

void DevConServerConnection::registerInterest(const String& id, const ConfigNode& params, uint32_t handle)
{
	queue->enqueue(std::make_unique<DevCon::RegisterInterestMsg>(id, ConfigNode(params), handle), 0);
}

void DevConServerConnection::updateInterest(uint32_t handle, const ConfigNode& params)
{
	queue->enqueue(std::make_unique<DevCon::UpdateInterestMsg>(handle, ConfigNode(params)), 0);
}

void DevConServerConnection::unregisterInterest(uint32_t handle)
{
	queue->enqueue(std::make_unique<DevCon::UnregisterInterestMsg>(handle), 0);
}

const std::optional<DevConClientInfo>& DevConServerConnection::getClientInfo() const
{
	return clientInfo;
}

String DevConServerConnection::getAddress() const
{
	return connection->getRemoteAddress();
}

Vector<DevCon::LogMsg> DevConServerConnection::movePendingLogs()
{
	auto result = std::move(pendingLogs);
	pendingLogs.clear();
	return result;
}

Future<ConfigNode> DevConServerConnection::sendRPC(String method, ConfigNode params)
{
	const auto id = rpcId++;
	queue->enqueue(std::make_unique<DevCon::RPCMsg>(id, std::move(method), std::move(params)), 0);

	Promise<ConfigNode> promise;
	auto future = promise.getFuture();
	pendingRPC[id] = std::move(promise);
	return future;
}


void DevConServerConnection::onReceiveMsg(DevCon::LogMsg& msg)
{
	pendingLogs.push_back(std::move(msg));
	//Logger::log(msg.level, "[REMOTE] " + msg.msg);
}

void DevConServerConnection::onReceiveMsg(DevCon::NotifyInterestMsg& msg)
{
	parent.onReceiveNotifyInterestMsg(*this, msg);
}

void DevConServerConnection::onReceiveMsg(DevCon::SetClientDataMsg& msg)
{
	DevConClientInfo info;
	info.platform = msg.platform;
	info.deviceName = msg.deviceName;
	info.params = msg.params;
	clientInfo = std::move(info);
}

void DevConServerConnection::onReceiveMsg(DevCon::RPCReplyMsg& msg)
{
	if (const auto iter = pendingRPC.find(msg.id); iter != pendingRPC.end()) {
		auto promise = std::move(iter->second);
		pendingRPC.erase(iter);
		promise.setValue(std::move(msg.result));
	}
}

DevConServer::DevConServer(std::unique_ptr<NetworkService> s, int port)
	: service(std::move(s))
{
	service->startListening([=] (NetworkService::Acceptor& a)
	{
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

void DevConServer::reloadAssets(Vector<String> assetIds, Vector<String> packIds)
{
	for (const auto& c: connections) {
		c->reloadAssets(std::move(assetIds), std::move(packIds));
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

void DevConServer::updateInterest(InterestHandle handle, ConfigNode params)
{
	if (interest[handle].config != params) {
		for (const auto& c : connections) {
			c->updateInterest(handle, params);
		}

		interest[handle].config = std::move(params);
	}
}

void DevConServer::unregisterInterest(InterestHandle handle)
{
	for (const auto& c: connections) {
		c->unregisterInterest(handle);
	}
	interest.erase(handle);
}

const ConfigNode& DevConServer::getInterestParams(InterestHandle handle) const
{
	return interest.at(handle).config;
}

gsl::span<std::shared_ptr<DevConServerConnection>> DevConServer::getConnections()
{
	return connections.span();
}

void DevConServer::setProject(IProject* project)
{
	this->project = project;
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
	Logger::logInfo("New incoming DevCon connection from " + conn.getAddress());
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
