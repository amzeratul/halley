#include "halley/entity/inspector.h"

#include "halley/devcon/devcon_client.h"
#include "halley/devcon/devcon_server.h"
#include "halley/entity/world.h"

using namespace Halley;

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
		ConfigNode worldNode;
		worldNode["name"] = world->getName();
		worldNode["uuid"] = world->getUUID().toString();
		worldNodes.push_back(std::move(worldNode));
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
					result["entity"] = getEntityData(world->getEntity(entityId));
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
			ConfigNode params;
			interestHandle = connection->getParent().registerInterest("inspector", std::move(params), [=](size_t idx, ConfigNode result)
			{
				onData(std::move(result));
			}, connection->getId());
		}

		this->listening = listening;
	}
}

void InspectorServer::onData(ConfigNode data)
{
	// TODO
}
