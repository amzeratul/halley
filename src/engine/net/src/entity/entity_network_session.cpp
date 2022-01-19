#include "entity/entity_network_session.h"

using namespace Halley;

EntityNetworkSession::EntityNetworkSession(std::shared_ptr<NetworkSession> session)
	: session(std::move(session))
{
}

void EntityNetworkSession::updateLocalEntities(World& world, gsl::span<const EntityId> entityIds)
{
	// TODO
}

void EntityNetworkSession::updateRemoteEntities(World& world, Resources& resources)
{
	// TODO
}
