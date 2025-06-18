#include <systems/network_send_system.h>

using namespace Halley;

class NetworkSendSystem final : public NetworkSendSystemBase<NetworkSendSystem> {

public:
	void init()
	{
		if (getSessionService().isMultiplayer()) {
			setupCheats();
		}
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
			
			const auto myPeerId = maybePeerId.value();
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
						Expects(myPeerId == 0);
						e.network.ownerId = myPeerId;
					} else {
						// Entities created locally belong to this peer. Entities loaded from world chunks are
						// supposed to be claimed by the host.
						Expects(myPeerId != 0);
						if (entity.getWorldPartition() == 0) {
							//Logger::logDev("Peer " + toString((int) myPeerId) + " is claiming network ownership for " + entity.getName());
							e.network.ownerId = myPeerId;
						} else {
							//Logger::logDev("Peer " + toString((int) myPeerId) + " assigns network ownership for " + entity.getName() + " (" + entity.getInstanceUUID() + ", world partition " + entity.getWorldPartition() + ") to host");
							e.network.ownerId = 0;
						}
					}

					e.network.creatorId = myPeerId;
				}

				if (e.network.sendUpdates && e.network.ownerId) {
					uint8_t ownerId = e.network.ownerId.value();
					uint8_t authorityId = e.network.authorityId.value_or(ownerId);
					if (ownerId == myPeerId || authorityId == myPeerId || isHost) {
						entities.emplace_back(EntityNetworkUpdateInfo{ e.entityId, ownerId, authorityId });
					}
				}
			}

			const auto viewPort = Rect4i(getScreenService().getCameraViewPort());
			
			entityNetworkSession.sendEntityUpdates(t, viewPort, myPeerId, entities);
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

	void setupCheats()
	{
		auto& consoleCommands = getDevService().getConsoleCommands();

		consoleCommands.addCommand("findNetworkEntityOutbound", [this](Vector<String> args) -> String
		{
			if (args.size() != 1 || !args[0].isInteger()) {
				return "Error: no or malformed network ID";
			}

			EntityNetworkId networkId(args[0].toInteger());
			String output = "";

			getSessionService().getMultiplayerSession().getEntityNetworkSession()->findEntity(networkId, false, [&](EntityId entityId, NetworkSession::PeerId peerId) {
				const auto e = getWorld().tryGetEntity(entityId);
				if (e.isValid()) {
					output += e.getName() + ", entity ID " + toString(entityId) + ", peer " + toString((int) peerId) + "\n";
				} else {
					output += "invalid entity ID " + toString(entityId) + " for network ID " + toString(networkId) + ", peer " + toString((int) peerId) + "\n";
				}
			});

			if (output.isEmpty()) {
				output += "no outbound entities found with network ID " + toString(networkId) + " for active peers\n";
			}

			return output;
		}, UIDebugConsoleSyntax());

		consoleCommands.addCommand("logNetworkEntityUpdates", [this](Vector<String> args) -> String
		{
			getSessionService().getMultiplayerSession().getEntityNetworkSession()->logUpdates();
			return "Ok.";
		}, UIDebugConsoleSyntax());
	}
};

REGISTER_SYSTEM(NetworkSendSystem)
