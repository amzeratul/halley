#include "entity/entity_network_session.h"

#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

EntityNetworkSession::EntityNetworkSession(std::shared_ptr<NetworkSession> session)
	: session(std::move(session))
{
	Expects(this->session);
	this->session->addListener(this);
}

EntityNetworkSession::~EntityNetworkSession()
{
	session->removeListener(this);
}

void EntityNetworkSession::init(World& world, Resources& resources)
{
	factory = std::make_shared<EntityFactory>(world, resources);
}

void EntityNetworkSession::sendLocalEntities(Time t, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
{
	for (auto& peer: peers) {
		peer.sendEntities(t, *factory, entityIds);
	}
}

void EntityNetworkSession::receiveRemoteEntities()
{
	for (auto& peer: peers) {
		peer.receiveEntities(*factory);
	}
}

void EntityNetworkSession::onStartSession(NetworkSession::PeerId myPeerId)
{
	// TODO
}

void EntityNetworkSession::onPeerConnected(NetworkSession::PeerId peerId)
{
	peers.push_back(EntityNetworkRemotePeer(peerId));
	Logger::logDev("Peer " + toString(static_cast<int>(peerId)) + " connected to EntityNetworkSession.");
}

void EntityNetworkSession::onPeerDisconnected(NetworkSession::PeerId peerId)
{
	Logger::logDev("Peer " + toString(static_cast<int>(peerId)) + " disconnected from EntityNetworkSession.");
	for (auto& peer: peers) {
		if (peer.getPeerId() == peerId) {
			peer.destroy(factory->getWorld());
		}
	}
	std_ex::erase_if(peers, [](const EntityNetworkRemotePeer& p) { return !p.isAlive(); });
}

