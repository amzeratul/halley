#include "halley/entity/inspector.h"

#include "halley/devcon/devcon_client.h"
#include "halley/devcon/devcon_server.h"
#include "halley/entity/world.h"

using namespace Halley;

InspectorWorldInfo::InspectorWorldInfo(const ConfigNode& node)
	: name(node["name"].asString())
	, uuid(node["uuid"].asString())
{
}

InspectorWorldInfo::InspectorWorldInfo(const World& world)
	: name(world.getName())
	, uuid(world.getUUID())
{
}

ConfigNode InspectorWorldInfo::toConfigNode() const
{
	ConfigNode result;
	result["name"] = name;
	result["uuid"] = uuid;
	return result;
}

bool InspectorWorldInfo::operator==(const InspectorWorldInfo& other) const
{
	return name == other.name && uuid == other.uuid;
}

bool InspectorWorldInfo::operator!=(const InspectorWorldInfo& other) const
{
	return !(*this == other);
}

InspectorClient::InspectorClient(DevConClient& devcon)
	: devcon(devcon)
{
}

InspectorClient::~InspectorClient()
{
	update({});
}

void InspectorClient::update(gsl::span<World*> worlds)
{
	if (devcon.getInterest().hasInterest("inspector")) {
		auto configs = devcon.getInterest().getInterestConfigs("inspector");
		for (size_t i = 0; i < configs.size(); ++i) {
			devcon.getInterest().notifyInterest("inspector", i, getInspectorData(configs[i], worlds));
		}
	}
}

ConfigNode InspectorClient::getInspectorData(const ConfigNode& params, gsl::span<World*> worlds)
{
	ConfigNode::SequenceType worldNodes;
	for (auto* world: worlds) {
		worldNodes.push_back(InspectorWorldInfo(*world).toConfigNode());
	}

	ConfigNode result;
	result["worlds"] = std::move(worldNodes);

	if (params.hasKey("world")) {
		const auto targetUUID = UUID(params["world"].asString());
		for (auto* world: worlds) {
			if (world->getUUID() == targetUUID) {
				result["world"] = getWorldData(*world);

				if (params.hasKey("entity")) {
					const auto entityId = params["entity"].asEntityId();
					if (entityId.isValid()) {
						result["entity"] = getEntityData(world->getEntity(entityId));
					}
				}
			}
		}
	}

	return result;
}

ConfigNode InspectorClient::getWorldData(World& world)
{
	ConfigNode result;
	// TODO
	return result;
}

ConfigNode InspectorClient::getEntityData(EntityRef entity)
{
	ConfigNode result;
	// TODO
	return result;
}

InspectorServer::InspectorServer(std::shared_ptr<DevConServerConnection> connection)
	: connection(std::move(connection))
{
}

InspectorServer::~InspectorServer()
{
	setListening(false);
}

void InspectorServer::setListening(bool listening)
{
	if (this->listening != listening) {
		if (interestHandle) {
			connection->getParent().unregisterInterest(*interestHandle);
			interestHandle = {};
		}

		if (listening) {
			interestHandle = connection->getParent().registerInterest("inspector", ConfigNode(params), [=](size_t idx, ConfigNode result)
			{
				onData(std::move(result));
			}, connection->getId());
		}

		this->listening = listening;
	}
}

void InspectorServer::setParams(ConfigNode params)
{
	this->params = std::move(params);
	if (listening) {
		connection->getParent().updateInterest(*interestHandle, ConfigNode(this->params));
	}
}

void InspectorServer::setWorldInfoCallback(WorldInfoCallback callback)
{
	worldInfoCallback = std::move(callback);
}

void InspectorServer::setWorldDataCallback(WorldDataCallback callback)
{
	worldDataCallback = std::move(callback);
}

void InspectorServer::onData(ConfigNode data)
{
	auto newWorldInfos = data["worlds"].asVector<InspectorWorldInfo>();

	if (newWorldInfos != worldInfos) {
		worldInfos = std::move(newWorldInfos);
		if (worldInfoCallback) {
			worldInfoCallback(worldInfos);
		}
	}
}
