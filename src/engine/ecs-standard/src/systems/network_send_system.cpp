#include <systems/network_send_system.h>

using namespace Halley;

class NetworkSendSystem final : public NetworkSendSystemBase<NetworkSendSystem> {

public:
	void init()
	{
		setupSession();
		sessionChangeToken = getSessionService().addSessionChangeCallback([=, this] (SessionService::ChangeData data) {
			setupSession();
		});
	}

	void update(Time t)
	{
		if (!getSessionService().isMultiplayer()) {
			return;
		}

		resetCounters();

		auto& mpSession = getSessionService().getMultiplayerSession();
		auto& entityNetworkSession = *mpSession.getEntityNetworkSession();
		const auto maybePeerId = mpSession.getNetworkSession()->getMyPeerId();
		if (!maybePeerId) {
			// Not ready
			return;
		}
		
		const auto myPeerId = maybePeerId.value();
		const bool isHost = mpSession.isHost();

		for (const auto& e: networkFamily) {
			++networkEntities;

			// By default, consider sending updates for all entities in the family,
			// *except* the ones we do not have authority for right now.
			e.network.sendUpdates = e.network.authorityId.value_or(myPeerId) == myPeerId;

			// Now disable updates for entities that are children of other entities with a
			// network component. This doesn't require recursion, and should be faster than
			// walking down the child hierarchy.
			if (e.network.sendUpdates) {
				auto parent = getWorld().getEntity(e.entityId).getParent();
				while (parent.isValid() && parent.isSerializable()) {
					if (parent.hasComponent<NetworkComponent>()) {
						e.network.sendUpdates = false;
						break;
					}
					parent = parent.getParent();
				}
			}
		}

		entities.clear();
		for (auto& e: networkFamily) {
			// Try to automatically assign a peerId to any NetworkComponent that hasn't been bound yet.
			// This is done for entities created locally; remote entities will be pre-populated.
			if (!e.network.ownerId) [[unlikely]] {
				auto entity = getWorld().getEntity(e.entityId);

				if (isHost) {
					// The host always claims ownership.
					HalleyAssertDev(myPeerId == 0);
					e.network.ownerId = myPeerId;
				} else {
					// Entities created locally belong to this peer. Entities loaded from world chunks are
					// supposed to be claimed by the host.
					HalleyAssertDev(myPeerId != 0);
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
					++networkEntitiesSending;
					entities.emplace_back(EntityNetworkUpdateInfo{e.entityId, ownerId, authorityId,
						e.network.alwaysSend, e.network.requiresEntityFrameModified });
				}
			}
		}

		const auto viewPort = Rect4i(getScreenService().getCameraViewPort());
		
		const auto stats = entityNetworkSession.sendEntityUpdates(t, viewPort, myPeerId, entities);
		entityNetworkSession.sendUpdates();
		entityNetworkSession.update(0.0);

		logCounters(t, stats);
	}

private:
	Vector<EntityNetworkUpdateInfo> entities;
	ListenerSetToken sessionChangeToken;

	void setupSession()
	{
		if (getSessionService().isMultiplayer()) {
			setupCheats();
		} else {
			clearCheats();
		}
	}

	void setupCheats() const
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

		consoleCommands.addCommand("networkLag", [this](Vector<String> args) -> String
		{
			if (args.empty() || args.size() > 2) {
				return "Syntax: networkLag <average> [<variance>]";
			}

			if (!args[0].isNumber() || (args.size() > 1 && !args[1].isNumber())) {
				return "Error: no numbers argument(s)";
			}

			float average = std::clamp(args[0].toFloat(), 0.0f, 1.0f);

			float variance = 0.0f;
			if (args.size() > 1) {
				variance = std::clamp(args[1].toFloat(), 0.0f, 1.0f);
			}

			if (variance > average * 0.5f) {
				return "Error: high variance, ignoring";
			}

			getSessionService().getMultiplayerSession().getNetworkSession()->simulateLatency(average, variance);

			return "Ok.";
		}, UIDebugConsoleSyntax());

		consoleCommands.addCommand("networkLogStats", [this](Vector<String> args) -> String
		{
			logStats = !logStats;
			return "Ok.";
		}, UIDebugConsoleSyntax());

		consoleCommands.addCommand("networkQuality", [this](Vector<String> args) -> String
		{
			if (args.size() != 2 || !args[0].isNumber() || !args[1].isNumber()) {
				return "Syntax: networkQuality <loss> <duplicate>";
			}

			float loss = std::clamp(args[0].toFloat(), 0.0f, 0.95f);
			float dupe = std::clamp(args[1].toFloat(), 0.0f, 0.95f);
			getSessionService().getMultiplayerSession().getNetworkSession()->simulateQuality(loss, dupe);

			return "Ok.";
		}, UIDebugConsoleSyntax());

		consoleCommands.addCommand("disconnect", [this](Vector<String> args) -> String
		{
			if (getSessionService().getMultiplayerSession().isHost()) {
				return "Not allowed on host.";
			}

			getSessionService().getMultiplayerSession().getNetworkSession()->close();
			return "Bye.";
		}, UIDebugConsoleSyntax());
	}

	void clearCheats() const
	{
		auto& consoleCommands = getDevService().getConsoleCommands();
		consoleCommands.removeCommand("findNetworkEntityOutbound");
		consoleCommands.removeCommand("logNetworkEntityUpdates");
		consoleCommands.removeCommand("networkLag");
		consoleCommands.removeCommand("networkLogStats");
		consoleCommands.removeCommand("networkQuality");
		consoleCommands.removeCommand("disconnect");
	}

	int networkEntities = 0;
	int networkEntitiesSending = 0;
	SendEntitiesStats displayStats;
	SendEntitiesStats accumStats;
	Time accumTime = 0;
	int accumCount = 0;
	mutable bool logStats = false;

	void resetCounters()
	{
		networkEntities = 0;
		networkEntitiesSending = 0;
	}

	void logCounters(Time t, const SendEntitiesStats& stats)
	{
		if (!logStats) {
			return;
		}

		accumStats += stats;
		accumTime += t;
		++accumCount;

		if (accumTime >= 1.0) {
			displayStats = accumStats;
			displayStats.avg(accumStats, accumCount);

			accumStats = {};
			accumTime = 0;
			accumCount = 0;
		}

		ScreenLogger::logScreen("networkEntities", networkEntities);
		ScreenLogger::logScreen("networkEntitiesSending", networkEntitiesSending);
		ScreenLogger::logScreen(" send: created", displayStats.nCreated);
		ScreenLogger::logScreen(" send: updated", displayStats.nUpdated);
		ScreenLogger::logScreen(" send: destroyed", displayStats.nDestroyed);
		ScreenLogger::logScreen(" update check: dormant", displayStats.nUpdateIdle);
		ScreenLogger::logScreen(" update check: fast", displayStats.nUpdateSameHash);
		ScreenLogger::logScreen(" update check: full", displayStats.nUpdateChecked);
		ScreenLogger::logScreen("stats.nCheckedAcquiredAuthority", displayStats.nCheckedAcquiredAuthority);
		ScreenLogger::logScreen("stats.nCheckedRelinquishedAuthority", displayStats.nCheckedRelinquishedAuthority);
		ScreenLogger::logScreen("stats.nCheckedRegular", displayStats.nCheckedRegular);
	}
};

REGISTER_SYSTEM(NetworkSendSystem)
