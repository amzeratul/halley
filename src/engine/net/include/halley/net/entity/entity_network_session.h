#pragma once

#include <memory>
#include <gsl/span>

#include "halley/data_structures/hash_map.h"
#include "halley/entity/entity.h"
#include "halley/time/halleytime.h"
#include "../session/network_session.h"

namespace Halley {
	class Resources;
	struct EntityId;
	class World;
	class NetworkSession;

	class EntityNetworkSession {
    public:
		explicit EntityNetworkSession(std::shared_ptr<NetworkSession> session);

		void sendLocalEntities(Time t, World& world, gsl::span<const std::pair<EntityId, uint8_t>> entityIds); // Takes pairs of entity id and owner peer id
		void receiveRemoteEntities(World& world, Resources& resources);

	private:
		class OutboundEntity {
		public:
			bool alive = true;
			uint16_t networkId = 0;
			Bytes data;
		};

		class InboundEntity {
			EntityId worldId;
			Bytes data;
		};

		class RemotePeer {
		public:
			RemotePeer(NetworkSession::PeerId peerId);

			NetworkSession::PeerId getPeerId() const;
			void updateEntities(Time t, World& world, gsl::span<const std::pair<EntityId, uint8_t>> entityIds);

		private:
			NetworkSession::PeerId peerId;
			HashMap<EntityId, OutboundEntity> outboundEntities;
			HashMap<EntityId, InboundEntity> inboundEntities;

			void createEntity(EntityRef entity);
			void destroyEntity(OutboundEntity& remote);
			void updateEntity(OutboundEntity& remote, EntityRef entity);
			uint16_t assignId();
		};

		std::shared_ptr<NetworkSession> session;
		std::vector<RemotePeer> peers;
    };
}
