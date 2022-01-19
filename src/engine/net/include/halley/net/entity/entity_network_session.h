#pragma once

#include <memory>
#include <gsl/span>

namespace Halley {
	class Resources;
	struct EntityId;
	class World;
	class NetworkSession;

	class EntityNetworkSession {
    public:
		explicit EntityNetworkSession(std::shared_ptr<NetworkSession> session);

		void updateLocalEntities(World& world, gsl::span<const EntityId> entityIds);
		void updateRemoteEntities(World& world, Resources& resources);

	private:
		std::shared_ptr<NetworkSession> session;
    };
}
