#include "devcon/devcon_client.h"
#include "halley/net/connection/network_service.h"
#include "halley/net/connection/message_queue_tcp.h"
#include "halley/support/logger.h"
#include "halley/core/api/halley_api.h"
#include "halley/net/connection/message_queue.h"
#include "devcon/devcon_messages.h"

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



DevConClient::DevConClient(const HalleyAPI& api, Resources& resources, std::unique_ptr<NetworkService> service, String address, int port)
	: api(api)
	, resources(resources)
	, service(std::move(service))
	, address(std::move(address))
	, port(port)
{
	interest = std::make_unique<DevConInterest>(*this);

	connect();

	Logger::addSink(*this);
}

DevConClient::~DevConClient()
{
	Logger::removeSink(*this);
	queue.reset();
	service.reset();
}

void DevConClient::update(Time t)
{
	queue->sendAll();
	service->update(t);

	for (auto& m: queue->receiveMessages()) {
		auto& msg = dynamic_cast<DevCon::DevConMessage&>(*m);
		switch (msg.getMessageType()) {
		case DevCon::MessageType::ReloadAssets:
			onReceiveReloadAssets(dynamic_cast<DevCon::ReloadAssetsMsg&>(msg));
			break;

		case DevCon::MessageType::RegisterInterest:
			onReceiveRegisterInterest(dynamic_cast<DevCon::RegisterInterestMsg&>(msg));
			break;

		case DevCon::MessageType::UnregisterInterest:
			onReceiveUnregisterInterest(dynamic_cast<DevCon::UnregisterInterestMsg&>(msg));
			break;

		default:
			break;
		}
	}
}

void DevConClient::onReceiveReloadAssets(const DevCon::ReloadAssetsMsg& msg)
{
	if (msg.ids.size() <= 5) {
		Logger::logDev("Reloading " + toString(msg.ids.size()) + " assets: " + toString(msg.ids));
	} else {
		Logger::logDev("Reloading " + toString(msg.ids.size()) + " assets.");
	}
	resources.reloadAssets(msg.ids);
}

void DevConClient::onReceiveRegisterInterest(DevCon::RegisterInterestMsg& msg)
{
	interest->registerInterest(msg.id, std::move(msg.params), msg.handle);
}

void DevConClient::onReceiveUnregisterInterest(const DevCon::UnregisterInterestMsg& msg)
{
	interest->unregisterInterest(msg.handle);
}

DevConInterest& DevConClient::getInterest() const
{
	return *interest;
}

void DevConClient::notifyInterest(uint32_t handle, ConfigNode data)
{
	queue->enqueue(std::make_unique<DevCon::NotifyInterestMsg>(handle, std::move(data)), 0);
}

void DevConClient::connect()
{
	queue = std::make_shared<MessageQueueTCP>(service->connect(address + ":" + toString(port)));
	DevCon::setupMessageQueue(*queue);
}

void DevConClient::log(LoggerLevel level, const String& msg)
{
	if (level != LoggerLevel::Dev) {
		if (queue->isConnected()) {
			queue->enqueue(std::make_unique<DevCon::LogMsg>(level, msg), 0);
		}
	}
}
