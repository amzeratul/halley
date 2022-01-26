#pragma once

#include "entity_network_header.h"
#include "halley/data_structures/hash_map.h"
#include "halley/entity/entity.h"
#include "../session/network_session.h"
#include "halley/entity/entity_factory.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class EntityClientSharedData;
	class EntityNetworkSession;
	struct EntityId;
    class EntityData;

    class EntityNetworkRemotePeer {
    public:
        EntityNetworkRemotePeer(EntityNetworkSession& parent, NetworkSession::PeerId peerId);

        NetworkSession::PeerId getPeerId() const;
        void sendEntities(Time t, gsl::span<const std::pair<EntityId, uint8_t>> entityIds, const EntityClientSharedData& clientData);
        void receiveEntityPacket(NetworkSession::PeerId fromPeerId, EntityNetworkHeaderType type, InboundNetworkPacket packet);

        void destroy();
        bool isAlive() const;

    private:
        class OutboundEntity {
        public:
            bool alive = true;
            Time timeSinceSend = 0;
            EntityNetworkId networkId = 0;
            EntityData data;
        };

        class InboundEntity {
        public:
            EntityId worldId;
            EntityData data;
        };

        EntityNetworkSession* parent = nullptr;
        NetworkSession::PeerId peerId;
    	bool alive = true;
        bool hasSentData = false;
    	
        HashMap<EntityId, OutboundEntity> outboundEntities;
        HashMap<EntityNetworkId, InboundEntity> inboundEntities;

    	HashSet<EntityNetworkId> allocatedOutboundIds;
        uint16_t nextId = 0;

        uint16_t assignId();
        void sendCreateEntity(EntityRef entity);
        void sendUpdateEntity(Time t, OutboundEntity& remote, EntityRef entity);
        void sendDestroyEntity(OutboundEntity& remote);
        void send(EntityNetworkHeaderType type, EntityNetworkId networkId, Bytes data);

        void receiveCreateEntity(EntityNetworkId id, gsl::span<const gsl::byte> data);
        void receiveUpdateEntity(EntityNetworkId id, gsl::span<const gsl::byte> data);
        void receiveDestroyEntity(EntityNetworkId id);

        void onFirstDataBatchSent();
	};
}