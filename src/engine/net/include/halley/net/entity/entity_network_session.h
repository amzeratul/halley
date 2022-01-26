#pragma once

#include <memory>
#include <gsl/span>

#include "halley/time/halleytime.h"
#include "../session/network_session.h"
#include "entity_network_remote_peer.h"
#include "halley/bytes/serialization_dictionary.h"

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

	class EntityNetworkSession : NetworkSession::IListener, NetworkSession::ISharedDataHandler {
    public:
		class IEntityNetworkSessionListener {
		public:
			virtual ~IEntityNetworkSessionListener() = default;
			virtual void onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId) {}
			virtual void onPreSendDelta(EntityDataDelta& delta) {}
			virtual bool isEntityInView(EntityRef entity, const EntityClientSharedData& clientData) = 0;
		};
		
		EntityNetworkSession(std::shared_ptr<NetworkSession> session, Resources& resources, std::set<String> ignoreComponents, IEntityNetworkSessionListener* listener);
		~EntityNetworkSession() override;

		void setWorld(World& world);
		
		void sendUpdates(Time t, Rect4i viewRect, gsl::span<const std::pair<EntityId, uint8_t>> entityIds); // Takes pairs of entity id and owner peer id
		void receiveUpdates();

		World& getWorld() const;
		EntityFactory& getFactory() const;
		NetworkSession& getSession() const;
		bool hasWorld() const;

		const EntityFactory::SerializationOptions& getEntitySerializationOptions() const;
		const EntityDataDelta::Options& getEntityDeltaOptions() const;
		const SerializerOptions& getByteSerializationOptions() const;

		Time getMinSendInterval() const;

		void onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId);
		void onPreSendDelta(EntityDataDelta& delta);

		bool isReadyToStart() const;
		bool isEntityInView(EntityRef entity, const EntityClientSharedData& clientData) const;

		std::vector<Rect4i> getRemoteViewPorts() const;

	protected:
		void onStartSession(NetworkSession::PeerId myPeerId) override;
		void onPeerConnected(NetworkSession::PeerId peerId) override;
		void onPeerDisconnected(NetworkSession::PeerId peerId) override;
		std::unique_ptr<SharedData> makeSessionSharedData() override;
		std::unique_ptr<SharedData> makePeerSharedData() override;
	
	private:
		struct QueuedPacket {
			NetworkSession::PeerId fromPeerId;
			EntityNetworkHeaderType type;
			InboundNetworkPacket packet;
		};
		
		Resources& resources;
		std::shared_ptr<EntityFactory> factory;
		IEntityNetworkSessionListener* listener = nullptr;
		
		EntityFactory::SerializationOptions entitySerializationOptions;
		EntityDataDelta::Options deltaOptions;
		SerializerOptions byteSerializationOptions;
		SerializationDictionary serializationDictionary;

		std::shared_ptr<NetworkSession> session;
		std::vector<EntityNetworkRemotePeer> peers;

		std::vector<QueuedPacket> queuedPackets;

		bool readyToStart = false;

		void onReceiveEntityUpdate(NetworkSession::PeerId fromPeerId, EntityNetworkHeaderType type, InboundNetworkPacket packet);
		void onReceiveReady(NetworkSession::PeerId fromPeerId);

		void setupDictionary();
	};
}
