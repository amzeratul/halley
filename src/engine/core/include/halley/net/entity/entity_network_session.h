#pragma once

#include <memory>
#include <gsl/span>

#include "halley/time/halleytime.h"
#include "../session/network_session.h"
#include "entity_network_remote_peer.h"
#include "halley/bytes/serialization_dictionary.h"
#include "halley/entity/system.h"
#include "halley/entity/world.h"
#include "halley/net/interpolators/byte_data_interpolator.h"

class NetworkComponent;

namespace Halley {
	class EntityFactory;
	class Resources;
	class World;
	class NetworkSession;

	class EntitySessionSharedData : public SharedData {
	public:
		bool gameStarted = false;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;
	};

	class EntityClientSharedData : public SharedData {
	public:
		std::optional<Rect4i> viewRect;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;
	};

	class EntityNetworkSession : NetworkSession::IListener, public IWorldNetworkInterface {
    public:
		class IEntityNetworkSessionListener {
		public:
			virtual ~IEntityNetworkSessionListener() = default;
			virtual void onStartSession(NetworkSession::PeerId myPeerId) = 0;
			virtual void onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId) {}
			virtual void setupInterpolators(DataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote) = 0;
			virtual void setupByteInterpolators(ByteDataInterpolatorSet& interpolatorSet) = 0;
			virtual void setupByteInterpolators(ByteDataInterpolatorSet& interpolatorSet, EntityRef entity) = 0;
			virtual bool allowComponentAddedForFastUpdate(uint16_t componentId) const { return false; }
			virtual bool isEntitySerializableAsChild(EntityRef entity, const World& world) const { return entity.isSerializable(); }
			virtual bool isEntityInView(EntityRef entity, const EntityClientSharedData& clientData, NetworkSession::PeerId peerId) = 0;
			virtual ConfigNode getLobbyInfo() = 0;
			virtual bool setLobbyInfo(NetworkSession::PeerId fromPeerId, const ConfigNode& lobbyInfo) = 0;
			virtual void onReceiveLobbyInfo(const ConfigNode& lobbyInfo) = 0;
		};
		
		EntityNetworkSession(std::shared_ptr<NetworkSession> session, Resources& resources, HashSet<String> ignoreComponents, IEntityNetworkSessionListener* listener);
		~EntityNetworkSession() override;

		void setWorld(World& world, SystemMessageBridge bridge);

		void update(Time t);
		void sendUpdates();
		void sendEntityUpdates(Time t, Rect4i viewRect, uint8_t myPeerId, gsl::span<const EntityNetworkUpdateInfo> entityIds);
		void receiveUpdates();

		World& getWorld() const;
        Resources& getResources() const;
		EntityFactory& getFactory() const;
		NetworkSession& getSession() const;
		bool hasWorld() const;

		const EntityFactory::SerializationOptions& getEntitySerializationOptions() const;
		const EntityDataDelta::Options& getEntityDeltaOptions() const;
		const SerializerOptions& getByteSerializationOptions() const;
		SerializationDictionary& getSerializationDictionary();
		const IByteDataInterpolatorSet* getByteDataInterpolatorSet() const;

		Time getMinSendInterval() const;

		void onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId);
		void requestSetupInterpolators(DataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote);
		void requestSetupByteDataInterpolators(ByteDataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote);
		void setupOutboundInterpolators(EntityRef entity);

		void startGame();
		void joinGame();
		bool isGameStarted() const;
		bool isReadyToStartGame() const;
		bool isLobbyReady() const;

		bool isEntityInView(EntityRef entity, const EntityClientSharedData& clientData, NetworkSession::PeerId peerId) const;
		Vector<Rect4i> getRemoteViewPorts() const;

		bool isHost() const override;
		bool isConnected() const override;
		bool isOwner(ConstEntityRef entity) const override;
		bool isAuthority(ConstEntityRef entity) const override;
		void sendEntityMessage(EntityRef entity, int messageType, Bytes messageData) override;
		void sendSystemMessage(String targetSystem, int messageType, Bytes messageData, SystemMessageDestination destination, SystemMessageCallback callback) override;

		void sendToAll(EntityNetworkMessage msg);
		void sendToPeer(EntityNetworkMessage msg, NetworkSession::PeerId peerId);

		void requestLobbyInfo();
		void setLobbyInfo(ConfigNode info);

		void findEntity(EntityNetworkId networkId, bool inbound, std::function<void(EntityId, NetworkSession::PeerId)> callback) const;
		void logUpdates();

		bool prepareChangeEntityAuthority(EntityId entityId, const NetworkComponent& networkComponent, std::optional<NetworkSession::PeerId> authorityId);
		bool allowComponentAddedForFastUpdate(uint16_t componentId) const;
		bool isEntitySerializableAsChild(EntityRef entity) const;

	protected:
		void onStartSession(NetworkSession::PeerId myPeerId) override;
		void onPeerConnected(NetworkSession::PeerId peerId) override;
		void onPeerDisconnected(NetworkSession::PeerId peerId) override;

	private:
		struct QueuedMessage {
			NetworkSession::PeerId fromPeerId;
			EntityNetworkMessage message;
		};

		struct PendingSysMsgResponse {
			SystemMessageCallback callback;
		};
		
		Resources& resources;
		std::shared_ptr<EntityFactory> factory;
		IEntityNetworkSessionListener* listener = nullptr;
		SystemMessageBridge messageBridge;
		uint32_t systemMessageId = 0;
		HashMap<uint32_t, PendingSysMsgResponse>  pendingSysMsgResponses;
		
		EntityFactory::SerializationOptions entitySerializationOptions;
		EntityDataDelta::Options deltaOptions;
		SerializerOptions byteSerializationOptions;
		SerializationDictionary serializationDictionary;

		std::shared_ptr<NetworkSession> session;
		Vector<EntityNetworkRemotePeer> peers;

		Vector<QueuedMessage> queuedPackets;

		HashMap<int, Vector<EntityNetworkMessage>> outbox;

		bool readyToStartGame = false;
		bool gameStarted = false;
		bool lobbyReady = false;

        Mutex outboundInterpolatorLock;
		ByteDataInterpolatorSet byteDataInterpolatorSet;

		Bytes receiveBuffer;
		HashMap<NetworkSession::PeerId, Bytes> largePacketBuffer;

		bool canProcessMessage(const EntityNetworkMessage& msg) const;
		void processMessage(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg);
		void onReceiveEntityUpdate(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg);
		void onReceiveReady(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageReadyToStart& msg);
		void onReceiveMessageToEntity(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageEntityMsg& msg);
		void onReceiveSystemMessage(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageSystemMsg& msg);
		void onReceiveSystemMessageResponse(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageSystemMsgResponse& msg);
		void onReceiveJoinWorld(NetworkSession::PeerId fromPeerId);
		void onReceiveGetLobbyInfo(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageGetLobbyInfo& msg);
		void onReceiveUpdateLobbyInfo(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageUpdateLobbyInfo& msg);
		void onReceiveSetLobbyInfo(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageSetLobbyInfo& msg);

		void sendMessages();
		
		void setupDictionary();
		void setupByteSerializationInterpolators();

		ConfigNode getLobbyInfo();
		void sendUpdatedLobbyInfos(std::optional<NetworkSession::PeerId> toPeerId);
	};
}
