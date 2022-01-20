#pragma once

#include <memory>
#include <gsl/span>

#include "halley/time/halleytime.h"
#include "../session/network_session.h"
#include "entity_network_remote_peer.h"

namespace Halley {
	class EntityFactory;
	class Resources;
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

		World* world = nullptr;
		Resources* resources = nullptr;
		std::shared_ptr<EntityFactory> factory;

		std::shared_ptr<NetworkSession> session;
		std::vector<EntityNetworkRemotePeer> peers;
	};
}
