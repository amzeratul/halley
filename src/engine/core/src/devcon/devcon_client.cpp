#include "halley/devcon/devcon_client.h"
#include "halley/net/connection/network_service.h"
#include "halley/net/connection/message_queue_tcp.h"
#include "halley/support/logger.h"
#include "halley/api/halley_api.h"
#include "halley/net/connection/message_queue.h"
#include "halley/devcon/devcon_messages.h"

using namespace Halley;


DevConInterest::DevConInterest(DevConClient& parent)
	: parent(parent)
{}

void DevConInterest::registerInterest(String id, ConfigNode config, uint32_t handle)
{
	auto& group = interests[id];
	group.configs.push_back(config);
	group.handles.push_back(handle);
	group.lastResults.push_back(ConfigNode());
}

void DevConInterest::updateInterest(uint32_t handle, ConfigNode config)
{
	for (auto& [k, v] : interests) {
		const auto iter = std::find(v.handles.begin(), v.handles.end(), handle);
		if (iter != v.handles.end()) {
			const auto idx = iter - v.handles.begin();
			v.configs[idx] = std::move(config);

			return;
		}
	}
}

void DevConInterest::unregisterInterest(uint32_t handle)
{
	for (auto& [k, v]: interests) {
		const auto iter = std::find(v.handles.begin(), v.handles.end(), handle);
		if (iter != v.handles.end()) {
			const auto idx = iter - v.handles.begin();
			v.handles.erase(iter);
			v.configs.erase(v.configs.begin() + idx);
			v.lastResults.erase(v.lastResults.begin() + idx);

			if (v.handles.empty()) {
				interests.erase(k);
			}

			return;
		}
	}
}

bool DevConInterest::hasInterest(const String& id) const
{
	return interests.contains(id);
}

gsl::span<const ConfigNode> DevConInterest::getInterestConfigs(const String& id) const
{
	const auto iter = interests.find(id);
	if (iter != interests.end()) {
		return iter->second.configs;
	}
	return {};
}

void DevConInterest::notifyInterest(const String& id, size_t configIdx, ConfigNode data)
{
	auto& group = interests.at(id);
	if (data != group.lastResults.at(configIdx)) {
		const auto handle = group.handles.at(configIdx);
		parent.notifyInterest(handle, ConfigNode(data));
		group.lastResults[configIdx] = std::move(data);
	}
}



DevConClient::DevConClient(const HalleyAPI& api, Resources& resources, std::unique_ptr<NetworkService> service)
	: api(api)
	, resources(resources)
	, service(std::move(service))
{
	interest = std::make_unique<DevConInterest>(*this);
}

DevConClient::~DevConClient()
{
	Logger::removeSink(*this);
	queue.reset();
	service.reset();
}

void DevConClient::update(Time t)
{
	if (!queue) {
		return;
	}

	queue->sendAll();
	service->update(t);

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

		default:
			break;
		}
	}
}

void DevConClient::onReceiveMessage(const DevCon::ReloadAssetsMsg& msg)
{
	if (msg.assetIds.size() <= 5) {
		Logger::logDev("Reloading " + toString(msg.assetIds.size()) + " assets: " + toString(msg.assetIds));
	} else {
		Logger::logDev("Reloading " + toString(msg.assetIds.size()) + " assets.");
	}
	resources.reloadAssets(msg.assetIds, msg.packIds);
}

void DevConClient::onReceiveMessage(DevCon::RegisterInterestMsg& msg)
{
	interest->registerInterest(msg.id, std::move(msg.params), msg.handle);
}

void DevConClient::onReceiveMessage(DevCon::UpdateInterestMsg& msg)
{
	interest->updateInterest(msg.handle, std::move(msg.params));
}

void DevConClient::onReceiveMessage(const DevCon::UnregisterInterestMsg& msg)
{
	interest->unregisterInterest(msg.handle);
}

void DevConClient::onReceiveMessage(DevCon::RPCMsg& msg)
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

DevConInterest& DevConClient::getInterest() const
{
	return *interest;
}

void DevConClient::setRPCHandle(const String& method, RPCHandle handle)
{
	rpcHandles[method] = std::move(handle);
}

void DevConClient::setDebugConsoleController(std::shared_ptr<UIDebugConsoleController> consoleController)
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

void DevConClient::notifyInterest(uint32_t handle, ConfigNode data)
{
	if (!queue) {
		return;
	}

	queue->enqueue(std::make_unique<DevCon::NotifyInterestMsg>(handle, std::move(data)), 0);
}

void DevConClient::connect(const String& deviceName, const ConfigNode& clientParams, const String& address, int port)
{
	if (queue) {
		Logger::logError("DevConClient already connected");
		return;
	}

	if (auto connection = service->connect(address + ":" + toString(port))) {
		queue = std::make_shared<MessageQueueTCP>(std::move(connection));
		DevCon::setupMessageQueue(*queue);

		queue->enqueue(std::make_unique<DevCon::SetClientDataMsg>(getPlatform(), deviceName, ConfigNode(clientParams)), 0);
		Logger::addSink(*this);
	}
}

void DevConClient::log(LoggerLevel level, const std::string_view msg)
{
	if (queue && queue->isConnected()) {
		queue->enqueue(std::make_unique<DevCon::LogMsg>(level, msg), 0);
	}
}
