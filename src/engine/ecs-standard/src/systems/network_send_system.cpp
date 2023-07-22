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

			// Enable only network components which aren't nested in another
			for (auto& e: networkFamily) {
				e.network.sendUpdates = true;
			}
			for (auto& e: networkFamily) {
				disableSendUpdateForChildren(getWorld().getEntity(e.entityId));
			}

			entities.clear();
			for (auto& e: networkFamily) {
				// Automatically assign the current peerId to any NetworkComponent that hasn't been bound yet
				// This means that all entities created locally belong to this peer; remote entities will be pre-populated
				if (!e.network.ownerId) {
					e.network.ownerId = peerId;
				}

				if ((e.network.ownerId == peerId || mpSession.isHost()) && e.network.sendUpdates) {
					entities.emplace_back(EntityNetworkUpdateInfo{ e.entityId, e.network.ownerId.value() });
				}

			}

			const auto viewPort = Rect4i(getScreenService().getCameraViewPort());
			
			entityNetworkSession.sendUpdates(t, viewPort, entities);
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
