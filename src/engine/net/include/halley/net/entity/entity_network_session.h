#pragma once

#include <memory>
#include <gsl/span>

#include "halley/time/halleytime.h"
#include "../session/network_session.h"
#include "entity_network_remote_peer.h"
#include "halley/bytes/serialization_dictionary.h"
#include "halley/entity/system.h"
#include "halley/entity/world.h"

namespace Halley {
	class EntityFactory;
	class Resources;
	class World;
	class NetworkSession;

	class EntitySessionSharedData : public SharedData {
	public:
		
	};

	class EntityClientSharedData : public SharedData {
	public:
		std::optional<Rect4i> viewRect;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;
	};

	class EntityNetworkSession : NetworkSession::IListener, NetworkSession::ISharedDataHandler, public IWorldNetworkInterface {
    public:
		class IEntityNetworkSessionListener {
		public:
			virtual ~IEntityNetworkSessionListener() = default;
			virtual void onStartSession(NetworkSession::PeerId myPeerId) = 0;
			virtual void onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId) {}
			virtual void setupInterpolators(DataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote) = 0;
			virtual void onPreSendDelta(EntityDataDelta& delta) {}
			virtual bool isEntityInView(EntityRef entity, const EntityClientSharedData& clientData) = 0;
		};
		
		EntityNetworkSession(std::shared_ptr<NetworkSession> session, Resources& resources, std::set<String> ignoreComponents, IEntityNetworkSessionListener* listener);
		~EntityNetworkSession() override;

		void setWorld(World& world, SystemMessageBridge bridge);
		
		void sendUpdates(Time t, Rect4i viewRect, gsl::span<const std::pair<EntityId, uint8_t>> entityIds); // Takes pairs of entity id and owner peer id
		void receiveUpdates();

		World& getWorld() const;
		EntityFactory& getFactory() const;
		NetworkSession& getSession() const;
		bool hasWorld() const;

		const EntityFactory::SerializationOptions& getEntitySerializationOptions() const;
		const EntityDataDelta::Options& getEntityDeltaOptions() const;
		const SerializerOptions& getByteSerializationOptions() const;
		SerializationDictionary& getSerializationDictionary();

		Time getMinSendInterval() const;

		void onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId);
		void onPreSendDelta(EntityDataDelta& delta);
		void requestSetupInterpolators(DataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote);
		void setupOutboundInterpolators(EntityRef entity);

		bool isReadyToStart() const;
		bool isEntityInView(EntityRef entity, const EntityClientSharedData& clientData) const;

		Vector<Rect4i> getRemoteViewPorts() const;

		bool isHost() override;
		bool isRemote(ConstEntityRef entity) const override;
		void sendEntityMessage(EntityRef entity, int messageType, Bytes messageData) override;
		void sendSystemMessage(String targetSystem, int messageType, Bytes messageData, SystemMessageDestination destination, SystemMessageCallback callback) override;

		void sendToAll(EntityNetworkMessage msg);
		void sendToPeer(EntityNetworkMessage msg, NetworkSession::PeerId peerId);

	protected:
		void onStartSession(NetworkSession::PeerId myPeerId) override;
		void onPeerConnected(NetworkSession::PeerId peerId) override;
		void onPeerDisconnected(NetworkSession::PeerId peerId) override;
		std::unique_ptr<SharedData> makeSessionSharedData() override;
		std::unique_ptr<SharedData> makePeerSharedData() override;
	
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

		bool readyToStart = false;

		bool canProcessMessage(const EntityNetworkMessage& msg) const;
		void processMessage(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg);
		void onReceiveEntityUpdate(NetworkSession::PeerId fromPeerId, EntityNetworkMessage msg);
		void onReceiveReady(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageReadyToStart& msg);
		void onReceiveMessageToEntity(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageEntityMsg& msg);
		void onReceiveSystemMessage(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageSystemMsg& msg);
		void onReceiveSystemMessageResponse(NetworkSession::PeerId fromPeerId, const EntityNetworkMessageSystemMsgResponse& msg);

		void sendMessages();
		
		void setupDictionary();
	};
}
