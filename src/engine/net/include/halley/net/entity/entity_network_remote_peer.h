#pragma once

#include "halley/data_structures/hash_map.h"
#include "halley/entity/entity.h"
#include "../session/network_session.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class EntityFactory;
	struct EntityId;
    class EntityData;

    class EntityNetworkRemotePeer {
    public:
        EntityNetworkRemotePeer(NetworkSession::PeerId peerId);

        NetworkSession::PeerId getPeerId() const;
        void sendEntities(Time t, EntityFactory& entityFactory, gsl::span<const std::pair<EntityId, uint8_t>> entityIds);
        void receiveEntities(EntityFactory& entityFactory);

        void destroy(World& world);
        bool isAlive() const;

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

        NetworkSession::PeerId peerId;
        HashMap<EntityId, OutboundEntity> outboundEntities;
        HashMap<NetworkEntityId, InboundEntity> inboundEntities;
        bool alive = true;

        void createEntity(EntityFactory& entityFactory, EntityRef entity);
        void destroyEntity(EntityFactory& entityFactory, OutboundEntity& remote);
        void updateEntity(EntityFactory& entityFactory, OutboundEntity& remote, EntityRef entity);
        uint16_t assignId();
    };
}