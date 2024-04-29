#pragma once

#include "entity_network_message.h"
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

    struct EntityNetworkUpdateInfo {
		EntityId entityId;
		uint8_t ownerId;
	};

    class EntityNetworkRemotePeer {
        constexpr static Time maxSendInterval = 1.0;
    	
    public:
        EntityNetworkRemotePeer(EntityNetworkSession& parent, NetworkSession::PeerId peerId);

        NetworkSession::PeerId getPeerId() const;

    	bool isAlive() const;
    	void destroy();

    	bool hasJoinedWorld() const;
        void onJoinedWorld();
        void requestJoinWorld();
        void requestAccountData(ConfigNode params);
        void sendAccountData(ConfigNode data);

    	void sendEntities(Time t, gsl::span<const EntityNetworkUpdateInfo> entityIds, const EntityClientSharedData& clientData);
        void receiveNetworkMessage(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg);

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
        bool joinedWorld = false;
    	
        HashMap<EntityId, OutboundEntity> outboundEntities;
        HashMap<EntityNetworkId, InboundEntity> inboundEntities;

    	HashSet<EntityNetworkId> allocatedOutboundIds;
        uint16_t nextId = 0;

        Time timeSinceSend = 0;

        uint16_t assignId();
        void sendCreateEntity(EntityRef entity);
        void sendUpdateEntity(Time t, OutboundEntity& remote, EntityRef entity);
        void sendDestroyEntity(OutboundEntity& remote);
        void sendKeepAlive();
        void send(EntityNetworkMessage message);

        void receiveCreateEntity(const EntityNetworkMessageCreate& msg);
        void receiveUpdateEntity(const EntityNetworkMessageUpdate& msg);
        void receiveDestroyEntity(const EntityNetworkMessageDestroy& msg);

        void destroyRemoteEntity(EntityId id);

        bool isRemoteReady() const;
        void onFirstDataBatchSent();

        void stripNestedNetworkComponents(EntityRef entity, int depth = 0);
	};
}