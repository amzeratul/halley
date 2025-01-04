#include "halley/devcon/devcon_connection.h"

#include "halley/net/connection/message_queue_tcp.h"
#include "halley/ui/widgets/ui_debug_console.h"

using namespace Halley;

DevConConnection::~DevConConnection()
{
	queue.reset();
	connection.reset();
}

void DevConConnection::setConnection(std::shared_ptr<IConnection> connection)
{
	this->connection = std::move(connection);

	queue = std::make_shared<MessageQueueTCP>(this->connection);
	queue->setChannel(0, ChannelSettings(true, true));

	queue->addFactory<DevCon::LogMsg>();
	queue->addFactory<DevCon::ReloadAssetsMsg>();
	queue->addFactory<DevCon::RegisterInterestMsg>();
	queue->addFactory<DevCon::UpdateInterestMsg>();
	queue->addFactory<DevCon::UnregisterInterestMsg>();
	queue->addFactory<DevCon::NotifyInterestMsg>();
	queue->addFactory<DevCon::SetClientDataMsg>();
	queue->addFactory<DevCon::RPCMsg>();
	queue->addFactory<DevCon::RPCReplyMsg>();
}

void DevConConnection::update(Time t)
{
	if (!queue) {
		return;
	}

	queue->sendAll();

	for (auto& m: queue->receiveMessages()) {
		auto& msg = dynamic_cast<DevCon::DevConMessage&>(*m);
		switch (msg.getMessageType()) {
		case DevCon::MessageType::ReloadAssets:
			onReceiveMessage(dynamic_cast<DevCon::ReloadAssetsMsg&>(msg));
			break;

		case DevCon::MessageType::RegisterInterest:
			onReceiveMessage(dynamic_cast<DevCon::RegisterInterestMsg&>(msg));
			break;

		case DevCon::MessageType::UpdateInterest:
			onReceiveMessage(dynamic_cast<DevCon::UpdateInterestMsg&>(msg));
			break;

		case DevCon::MessageType::UnregisterInterest:
			onReceiveMessage(dynamic_cast<DevCon::UnregisterInterestMsg&>(msg));
			break;

		case DevCon::MessageType::RPC:
			onReceiveMessage(dynamic_cast<DevCon::RPCMsg&>(msg));
			break;

		case DevCon::MessageType::Log:
			onReceiveMessage(dynamic_cast<DevCon::LogMsg&>(msg));
			break;

		case DevCon::MessageType::NotifyInterest:
			onReceiveMessage(dynamic_cast<DevCon::NotifyInterestMsg&>(msg));
			break;

		case DevCon::MessageType::SetClientData:
			onReceiveMessage(dynamic_cast<DevCon::SetClientDataMsg&>(msg));
			break;

		case DevCon::MessageType::RPCReply:
			onReceiveMessage(dynamic_cast<DevCon::RPCReplyMsg&>(msg));

		default:
			break;
		}
	}
}

Future<ConfigNode> DevConConnection::sendRPC(String method, ConfigNode params)
{
	const auto id = rpcId++;
	queue->enqueue(std::make_unique<DevCon::RPCMsg>(id, std::move(method), std::move(params)), 0);

	Promise<ConfigNode> promise;
	auto future = promise.getFuture();
	pendingRPC[id] = std::move(promise);
	return future;
}

void DevConConnection::setRPCHandle(const String& method, RPCHandle handle)
{
	rpcHandles[method] = std::move(handle);
}

void DevConConnection::setDebugConsoleController(std::shared_ptr<UIDebugConsoleController> consoleController)
{
	auto weakPtr = std::weak_ptr<UIDebugConsoleController>(consoleController);

	setRPCHandle("consoleCommand", [weakPtr] (ConfigNode params) -> Future<ConfigNode>
	{
		if (auto consoleController = weakPtr.lock()) {
			return consoleController->runCommand(params["command"].asString(), params["args"].asVector<String>()).then([] (UIDebugConsoleResponse result)
			{
				return result.toConfigNode();
			});
		} else {
			return Future<ConfigNode>::makeImmediate(ConfigNode("<UIDebugConsoleController expired>"));
		}
	});
}

String DevConConnection::getAddress() const
{
	return connection->getRemoteAddress();
}

bool DevConConnection::isAlive() const
{
	return connection->getStatus() != ConnectionStatus::Closed && connection->getStatus() != ConnectionStatus::Closing;
}


void DevConConnection::onReceiveMessage(DevCon::RPCMsg& msg)
{
	if (const auto iter = rpcHandles.find(msg.method); iter != rpcHandles.end()) {
		auto id = msg.id;
		iter->second(std::move(msg.params)).then([this, id] (ConfigNode result)
		{
			queue->enqueue(std::make_unique<DevCon::RPCReplyMsg>(id, std::move(result)), 0);
		});
	} else {
		queue->enqueue(std::make_unique<DevCon::RPCReplyMsg>(msg.id, UIDebugConsoleResponse("<DevCon has no handle registered for RPC \"" + msg.method +"\">").toConfigNode()), 0);
	}
}

void DevConConnection::onReceiveMessage(DevCon::RPCReplyMsg& msg)
{
	if (const auto iter = pendingRPC.find(msg.id); iter != pendingRPC.end()) {
		auto promise = std::move(iter->second);
		pendingRPC.erase(iter);
		promise.setValue(std::move(msg.result));
	}
}
