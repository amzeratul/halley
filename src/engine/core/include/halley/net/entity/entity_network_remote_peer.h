#pragma once

#include "entity_network_message.h"
#include "halley/data_structures/hash_map.h"
#include "halley/entity/entity.h"
#include "../session/network_session.h"
#include "halley/entity/entity_factory.h"
#include "halley/time/halleytime.h"
#include "entity_network_serialize.h"

namespace Halley {
	class EntityClientSharedData;
	class EntityNetworkSession;
	struct EntityId;
    class EntityData;
    
    struct SendEntitiesStats {
        int nCheckedAcquiredAuthority = 0;
        int nCheckedRelinquishedAuthority = 0;
        int nCheckedRegular = 0;
        int nUpdateChecked = 0;
        int nCreated = 0;
        int nUpdated = 0;
        int nDestroyed = 0;

		SendEntitiesStats& operator+=(const SendEntitiesStats& other)
		{
			nCheckedAcquiredAuthority += other.nCheckedAcquiredAuthority;
			nCheckedRelinquishedAuthority += other.nCheckedRelinquishedAuthority;
			nCheckedRegular += other.nCheckedRegular;
			nUpdateChecked += other.nUpdateChecked;
			nCreated += other.nCreated;
			nUpdated += other.nUpdated;
			nDestroyed += other.nDestroyed;
			return *this;
		}
		
		SendEntitiesStats operator+(const SendEntitiesStats& other) const
		{
			SendEntitiesStats result;
			result.nCheckedAcquiredAuthority = nCheckedAcquiredAuthority + other.nCheckedAcquiredAuthority;
			result.nCheckedRelinquishedAuthority = nCheckedRelinquishedAuthority + other.nCheckedRelinquishedAuthority;
			result.nCheckedRegular = nCheckedRegular + other.nCheckedRegular;
			result.nUpdateChecked = nUpdateChecked + other.nUpdateChecked;
			result.nCreated = nCreated + other.nCreated;
			result.nUpdated = nUpdated + other.nUpdated;
			result.nDestroyed = nDestroyed + other.nDestroyed;
			return result;
		}
    };

    struct EntityNetworkUpdateInfo {
		EntityId entityId;
		uint8_t ownerId;
    	uint8_t authorityId;
    	bool alwaysSend;
	};

	struct EntityNetworkInstanceInfo {
		UUID instanceUUID;
		UUID parentUUID;
		Serializer& serialize(Serializer& s) const;
		Deserializer& deserialize(Deserializer& s);
	};

    class EntityNetworkRemotePeer {
        constexpr static Time maxSendInterval = 1.0;
    	
    public:
        EntityNetworkRemotePeer(EntityNetworkSession& parentSession, NetworkSession::PeerId peerId);

        NetworkSession::PeerId getPeerId() const;

    	bool isAlive() const;
    	void destroy();
        void update(Time dt);

    	bool hasJoinedWorld() const;
        void onJoinedWorld();
        void requestJoinWorld();
        void requestLobbyInfo();
        void sendLobbyInfo(ConfigNode data);
        void setLobbyInfo(ConfigNode info);

    	SendEntitiesStats sendEntities(Time t, uint8_t myPeerId, gsl::span<const EntityNetworkUpdateInfo> entityIds, const EntityClientSharedData& clientData);
        void receiveNetworkMessage(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg);

    	[[nodiscard]] EntityId findInboundEntity(EntityNetworkId networkId) const;
    	[[nodiscard]] EntityId findOutboundEntity(EntityNetworkId networkId) const;

    	void logUpdates();

    	[[nodiscard]] std::pair<bool, std::optional<EntityNetworkId>> prepareChangeEntityAuthority(EntityId entityId, NetworkSession::PeerId myPeerId, NetworkSession::PeerId ownerId,
    		const std::optional<NetworkSession::PeerId>& authorityId, const std::optional<EntityNetworkId>& assignNetworkId);

    private:
        class OutboundEntity {
        public:
            bool alive = true;
        	bool hasAuthorityOnly = false;
        	bool forChildEntityTemporaryOnly = false;
        	bool forceNextFastUpdate = false;
            EntityNetworkId networkId = 0;
            Time timeSinceSend = 0;
            EntityData data;
            Bytes fastUpdateJournal;
        };

        class InboundEntity {
        public:
            EntityId worldId;
            EntityData data;
        	Vector<std::pair<Vector2f, int32_t>> positionUpdates;
        	bool appliedOnExistingEntity = false;
        	bool forChangedAuthorityOnly = false;
        	String debugName; // for debug only, do a proper lookup if needed
        };

        class PendingEntity {
        public:
            EntityNetworkId id;
        	EntityNetworkInstanceInfo instanceInfo;
	        std::optional<EntityDataDelta> data;
        	WorldPartitionId worldPartition;
            Vector<EntityNetworkMessageUpdate> updates;
        };

        EntityNetworkSession* parentSession = nullptr;
        NetworkSession::PeerId peerId;
    	bool alive = true;
        bool hasSentData = false;
        bool joinedWorld = false;
    	
        HashMap<EntityId, OutboundEntity> outboundEntities;
        HashMap<EntityNetworkId, InboundEntity> inboundEntities;
        HashMap<EntityNetworkId, InboundEntity> tempInboundEntities;
        HashMap<EntityNetworkId, PendingEntity> pendingEntities;

    	HashSet<EntityNetworkId> allocatedOutboundIds;
        uint16_t nextId = 0;

        Time timeSinceSend = 0;
    	bool log = false;

    	EntityNetworkSerialize fastSerializer;
        static thread_local Bytes fastUpdateOutboundData;

        uint16_t assignId();
        void sendCreateEntity(const EntityRef& entity);
        bool sendUpdateEntity(Time t, int32_t sessionTimestamp, OutboundEntity& remote, EntityRef entity);
        void sendDestroyEntity(OutboundEntity& remote, EntityId entityId);
        void sendKeepAlive();
        void send(EntityNetworkMessage message);

        void receiveCreateEntity(const EntityNetworkMessageCreate& msg);
        void receiveAssignEntity(const EntityNetworkMessageCreate& msg);
        void receiveUpdateEntity(const EntityNetworkMessageUpdate& msg);
        void receiveDestroyEntity(const EntityNetworkMessageDestroy& msg);

        EntityRef createRemoteEntity(EntityNetworkId id, const EntityDataDelta& delta, bool allowExistingLookup);
    	void assignRemoteEntity(EntityNetworkId id, EntityRef entity);
        void updateRemoteEntity(InboundEntity& inboundEntity, EntityRef entity, const EntityNetworkMessageUpdate& msg);
        void destroyRemoteEntity(const InboundEntity& inboundEntity);

        void trySpawningPendingEntities();
        void createPendingEntity(const PendingEntity& pendingData);
        void assignPendingEntity(const PendingEntity& pendingData, EntityRef entity);
        void updatePendingEntity(PendingEntity& entity, const EntityNetworkMessageUpdate& msg);

        bool isRemoteReady() const;
        void onFirstDataBatchSent();

    	void updateRemoteEntityPosition(InboundEntity& inboundEntity, const Vector2f& position, int32_t timestamp);
    	void interpolateRemoteEntityPosition(InboundEntity& inboundEntity, int32_t now, int32_t latency);
    	void interpolateRemoteEntityPositions(Time dt);
	};
}