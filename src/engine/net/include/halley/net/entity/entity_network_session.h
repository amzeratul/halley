#pragma once

#include <memory>
#include <gsl/span>

#include "halley/time/halleytime.h"

namespace Halley {
	class Resources;
	struct EntityId;
	class World;
	class NetworkSession;

	class EntityNetworkSession {
    public:
		explicit EntityNetworkSession(std::shared_ptr<NetworkSession> session);

		void updateLocalEntities(Time t, World& world, gsl::span<const std::pair<EntityId, uint8_t>> entityIds); // Receives pairs of entity id and owner peer id
		void updateRemoteEntities(World& world, Resources& resources);

	private:
		std::shared_ptr<NetworkSession> session;
    };
}
