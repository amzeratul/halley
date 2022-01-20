#pragma once

#include <memory>
#include <gsl/span>

#include "halley/data_structures/hash_map.h"
#include "halley/entity/entity.h"
#include "halley/time/halleytime.h"
#include "../session/network_session.h"

namespace Halley {
	class EntityFactory;
	class Resources;
	struct EntityId;
	class World;
	class NetworkSession;

	class EntityNetworkSession : NetworkSession::Listener {
    public:
		explicit EntityNetworkSession(std::shared_ptr<NetworkSession> session);
		~EntityNetworkSession();

		void init(World& world, Resources& resources);

		void sendLocalEntities(Time t, gsl::span<const std::pair<EntityId, uint8_t>> entityIds); // Takes pairs of entity id and owner peer id
		void receiveRemoteEntities();

	protected:
		void onStartSession(NetworkSession::PeerId myPeerId) override;
		void onPeerConnected(NetworkSession::PeerId peerId) override;
		void onPeerDisconnected(NetworkSession::PeerId peerId) override;

	private:
		using NetworkEntityId = uint16_t;
		
		class OutboundEntity {
		public:
			bool alive = true;
			NetworkEntityId networkId = 0;
			EntityData data;
		};

		class InboundEntity {
		public:
			EntityId worldId;
			EntityData data;
		};

		class RemotePeer {
		public:
			RemotePeer(NetworkSession::PeerId peerId);

			NetworkSession::PeerId getPeerId() const;
			void sendEntities(Time t, EntityFactory& entityFactory, gsl::span<const std::pair<EntityId, uint8_t>> entityIds);
			void receiveEntities(EntityFactory& entityFactory);

			void destroy(World& world);
			bool isAlive() const;

		private:
			NetworkSession::PeerId peerId;
			HashMap<EntityId, OutboundEntity> outboundEntities;
			HashMap<NetworkEntityId, InboundEntity> inboundEntities;
			bool alive = true;

			void createEntity(EntityFactory& entityFactory, EntityRef entity);
			void destroyEntity(EntityFactory& entityFactory, OutboundEntity& remote);
			void updateEntity(EntityFactory& entityFactory, OutboundEntity& remote, EntityRef entity);
			uint16_t assignId();
		};

		World* world = nullptr;
		Resources* resources = nullptr;
		std::shared_ptr<EntityFactory> factory;

		std::shared_ptr<NetworkSession> session;
		std::vector<RemotePeer> peers;
	};
}
