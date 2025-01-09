#include <systems/network_send_system.h>

using namespace Halley;

class NetworkSendSystem final : public NetworkSendSystemBase<NetworkSendSystem> {

public:
	void init()
	{
	}

	void update(Time t)
	{
		if (getSessionService().isMultiplayer()) {
			auto& mpSession = getSessionService().getMultiplayerSession();
			auto& entityNetworkSession = *mpSession.getEntityNetworkSession();
			const auto maybePeerId = mpSession.getNetworkSession()->getMyPeerId();
			if (!maybePeerId) {
				// Not ready
				return;
			}
			
			const auto peerId = maybePeerId.value();
			const bool isHost = mpSession.isHost();

			// Enable only network components which aren't nested in another
			for (const auto& e: networkFamily) {
				e.network.sendUpdates = true;
				disableSendUpdateForChildren(getWorld().getEntity(e.entityId));
			}

			entities.clear();
			for (auto& e: networkFamily) {
				// Try to automatically assign a peerId to any NetworkComponent that hasn't been bound yet.
				// This is done for entities created locally; remote entities will be pre-populated.
				if (!e.network.ownerId) {
					auto entity = getWorld().getEntity(e.entityId);

					if (isHost) {
						// The host always claims ownership.
						Expects(peerId == 0);
						//Logger::logDev("Host is claiming network ownership for " + entity.getName());
						e.network.ownerId = peerId;
					} else {
						// Entities created locally belong to this peer. Entities loaded from world chunks are
						// supposed to be claimed by the host.
						if (entity.getWorldPartition() == 0) {
							Logger::logDev("Peer " + toString((int) peerId) + " is claiming network ownership for " + entity.getName());
							e.network.ownerId = peerId;
						} else {
							Logger::logDev("Peer " + toString((int) peerId) + " assigns network ownership for " + entity.getName() + " to host");
							e.network.ownerId = 0;
						}
					}
				}

				if (e.network.sendUpdates && e.network.ownerId && (e.network.ownerId == peerId || isHost)) {
					entities.emplace_back(EntityNetworkUpdateInfo{ e.entityId, e.network.ownerId.value() });
				}
			}

			const auto viewPort = Rect4i(getScreenService().getCameraViewPort());
			
			entityNetworkSession.sendEntityUpdates(t, viewPort, entities);
			entityNetworkSession.sendUpdates();
			entityNetworkSession.update(0.0);
		}
	}

private:
	Vector<EntityNetworkUpdateInfo> entities;

	void disableSendUpdateForChildren(EntityRef entity)
	{
		for (auto c: entity.getChildren()) {
			if (auto* network = c.tryGetComponent<NetworkComponent>()) {
				network->sendUpdates = false;
			}

			disableSendUpdateForChildren(c);
		}
	}
};

REGISTER_SYSTEM(NetworkSendSystem)
