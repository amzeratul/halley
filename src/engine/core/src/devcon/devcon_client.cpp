#include "halley/devcon/devcon_client.h"
#include "halley/net/connection/network_service.h"
#include "halley/net/connection/message_queue_tcp.h"
#include "halley/support/logger.h"
#include "halley/api/halley_api.h"
#include "halley/net/connection/message_queue.h"
#include "halley/devcon/devcon_messages.h"
#include "halley/support/profiler.h"

using namespace Halley;


DevConInterest::DevConInterest(DevConClient& parent)
	: parent(parent)
{}

void DevConInterest::registerInterest(String id, ConfigNode config, uint32_t handle)
{
	UniqueLock lock(mutex);
	auto& group = interests[id];
	group.configs.push_back(config);
	group.handles.push_back(handle);
	group.lastResults.push_back(ConfigNode());
}

void DevConInterest::updateInterest(uint32_t handle, ConfigNode config)
{
	UniqueLock lock(mutex);
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
	UniqueLock lock(mutex);
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
	UniqueLock lock(mutex);
	return interests.contains(id);
}

gsl::span<const ConfigNode> DevConInterest::getInterestConfigs(const String& id) const
{
	UniqueLock lock(mutex);

	const auto iter = interests.find(id);
	if (iter != interests.end()) {
		return iter->second.configs;
	}
	return {};
}

void DevConInterest::notifyInterest(const String& id, size_t configIdx, ConfigNode data)
{
	UniqueLock lock(mutex);

	const auto iter = interests.find(id);
	if (iter != interests.end()) {
		auto& group = iter->second;
		if (data != group.lastResults.at(configIdx)) {
			const auto handle = group.handles.at(configIdx);
			Concurrent::execute(Executors::getMainUpdateThread(), [this, handle, data = ConfigNode(data)]() mutable
			{
				parent.notifyInterest(handle, std::move(data));
			});
			group.lastResults[configIdx] = std::move(data);
		}
	}
}



DevConClient::DevConClient(const HalleyAPI& api, Resources& resources, std::unique_ptr<NetworkService> service)
	: api(api)
	, resources(resources)
	, service(std::move(service))
{
	interest = std::make_unique<DevConInterest>(*this);

	setRPCHandle("pullSave", [=] (ConfigNode params) -> Future<ConfigNode> {
		const auto name = params["name"].asString();

		auto data = api.system->getStorageContainer(SaveDataType::SaveRoaming)->getData(name);
		Logger::logInfo("Pulling save file \"" + name + "\": " + String::prettySize(data.size()));

		return Future<ConfigNode>::makeImmediate(ConfigNode(std::move(data)));
	});

	setRPCHandle("pushSave", [=] (ConfigNode params) -> Future<ConfigNode> {
		const auto name = params["name"].asString();
		const auto data = params["data"].asBytes();

		Logger::logInfo("Pushing save file \"" + name + "\": " + String::prettySize(data.size()));
		api.system->getStorageContainer(SaveDataType::SaveRoaming)->setData(name, data, true);
		++pushSaveFileVersion;

		return Future<ConfigNode>::makeImmediate(ConfigNode(true));
	});
}

DevConClient::~DevConClient()
{
	if (receivingProfilerData) {
		api.core->removeProfilerCallback(this);
	}

	Logger::removeSink(*this);

	setConnection({});
}

void DevConClient::update(Time t)
{
	service->update(t);
	DevConConnection::update(t);

	const bool shouldHaveProfilerData = interest->hasInterest("profiler");
	if (shouldHaveProfilerData != receivingProfilerData) {
		if (shouldHaveProfilerData) {
			api.core->addProfilerCallback(this);
		} else {
			api.core->removeProfilerCallback(this);
		}
		receivingProfilerData = shouldHaveProfilerData;
	}

	if (i18n && interest->hasInterest("i18n")) {
		updateI18NInterest();
	}

	if (queue && !queue->isConnected()) {
		setConnection({});
		Logger::logDev("DevConClient: connection lost");
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

void DevConClient::onReceiveMessage(DevCon::UpdateStringsMsg& msg)
{
	if (i18n) {
		applyUpdateStrings(msg);
	} else {
		pendingUpdateStringMessages.emplace_back(std::move(msg));
	}
}

DevConInterest& DevConClient::getInterest() const
{
	return *interest;
}

void DevConClient::setI18N(I18N* newI18n)
{
	i18n = newI18n;

	if (i18n) {
		while (!pendingUpdateStringMessages.empty()) {
			applyUpdateStrings(pendingUpdateStringMessages.front());
			pendingUpdateStringMessages.erase(pendingUpdateStringMessages.begin());
		}
	}
}

uint32_t DevConClient::getPushSaveFileVersion() const
{
	return pushSaveFileVersion.load();
}

void DevConClient::notifyInterest(uint32_t handle, ConfigNode data)
{
	if (!queue) {
		return;
	}

	queue->enqueue(std::make_unique<DevCon::NotifyInterestMsg>(handle, std::move(data)), 0);
}

void DevConClient::onProfileData(std::shared_ptr<ProfilerData> data)
{
	if (interest->hasInterest("profiler")) {
		Concurrent::execute([=] ()
		{
			auto bytes = Serializer::toBytes(*data, SerializerOptions(SerializerOptions::maxVersion));
			interest->notifyInterest("profiler", 0, ConfigNode(std::move(bytes)));
		});
	}
}

void DevConClient::connect(const String& deviceName, const ConfigNode& clientParams, const String& address, int port)
{
	if (queue) {
		Logger::logError("DevConClient already connected");
		return;
	}

	if (auto connection = service->connect(address + ":" + toString(port))) {
		setConnection(connection);

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

void DevConClient::applyUpdateStrings(DevCon::UpdateStringsMsg& msg)
{
	assert(i18n != nullptr);

	i18n->updateStrings(msg.language, std::move(msg.strings));
}

void DevConClient::updateI18NInterest()
{
	assert(i18n != nullptr);

	ConfigNode data;
	data["languageCode"] = i18n->getCurrentLanguage().getISOCode();
	interest->notifyInterest("i18n", 0, std::move(data));
}
