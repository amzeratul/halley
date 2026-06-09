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

InspectorEntityInfo::InspectorEntityInfo(const ConfigNode& node)
{
	id = node["id"].asEntityId();
	parentId = node["parentId"].asEntityId();
	partition = node["partition"].asInt();
	name = node["name"].asString();
}

InspectorEntityInfo::InspectorEntityInfo(ConstEntityRef entity)
{
	id = entity.getEntityId();
	parentId = entity.hasParent() ? entity.getParent().getEntityId() : EntityId();
	partition = entity.getWorldPartition();
	name = entity.getName();
}

ConfigNode InspectorEntityInfo::toConfigNode() const
{
	ConfigNode result;
	result["id"] = id;
	result["parentId"] = parentId;
	result["partition"] = partition;
	result["name"] = name;
	return result;
}

bool InspectorEntityInfo::operator==(const InspectorEntityInfo& other) const
{
	return id == other.id && parentId == other.parentId && partition == other.partition && name == other.name;
}

bool InspectorEntityInfo::operator!=(const InspectorEntityInfo& other) const
{
	return !(*this == other);
}

InspectorWorldData::InspectorWorldData(const ConfigNode& node)
{
	entities = node["entities"].asVector<InspectorEntityInfo>({});
}

InspectorWorldData::InspectorWorldData(const World& world)
{
	// TODO: gotta make this more efficient
	return;

	const auto& worldEntities = world.getEntities();
	entities.reserve(worldEntities.size());
	for (const auto& entity: worldEntities) {
		entities.push_back(InspectorEntityInfo(entity).toConfigNode());
	}
}

ConfigNode InspectorWorldData::toConfigNode() const
{
	ConfigNode result;
	result["entities"] = entities;
	return result;
}

bool InspectorWorldData::operator==(const InspectorWorldData& other) const
{
	return entities == other.entities;
}

bool InspectorWorldData::operator!=(const InspectorWorldData& other) const
{
	return !(*this == other);
}

InspectorEntityData::InspectorEntityData(const ConfigNode& node)
{
	// TODO
}

InspectorEntityData::InspectorEntityData(EntityRef entity)
{
	// TODO
}

ConfigNode InspectorEntityData::toConfigNode() const
{
	ConfigNode result;
	// TODO
	return result;
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
		const auto targetUUID = UUID::tryParse(params["world"].asString({})).value_or(UUID());
		for (auto* world: worlds) {
			if (world->getUUID() == targetUUID) {
				result["world"] = InspectorWorldData(*world).toConfigNode();

				if (params.hasKey("entity")) {
					const auto entityId = params["entity"].asEntityId({});
					if (entityId.isValid()) {
						result["entity"] = InspectorEntityData(world->getEntity(entityId)).toConfigNode();
					}
				}
			}
		}
	}

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
			interestHandle = connection->getParent().registerInterest("inspector", ConfigNode(params), [=, this](size_t idx, ConfigNode result)
			{
				onData(std::move(result));
			}, connection->getId());
		}

		this->listening = listening;
	}
}

void InspectorServer::setParams(ConfigNode params)
{
	if (params != this->params) {
		this->params = std::move(params);
		if (listening) {
			connection->getParent().updateInterest(*interestHandle, ConfigNode(this->params));
		}
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

	auto newWorldData = InspectorWorldData(data["world"]);
	if (newWorldData != worldData) {
		worldData = std::move(newWorldData);
		if (worldDataCallback) {
			worldDataCallback(worldData);
		}
	}
}
