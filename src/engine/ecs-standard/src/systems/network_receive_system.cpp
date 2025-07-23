#include <systems/network_receive_system.h>

using namespace Halley;

class NetworkReceiveSystem final : public NetworkReceiveSystemBase<NetworkReceiveSystem> {
public:
	void init()
	{
		updateSession();
		sessionChangeToken = getSessionService().addSessionChangeCallback([=] (SessionService::ChangeData session) {
			updateSession();
		});
	}

	void update(Time t)
	{
		auto& world = getWorld();
		const bool sessionAlive = getSessionService().getSession().update(t);

		if (!sessionAlive && !requestedExit) {
			requestedExit = true;
			if (auto* exitGameInterface = world.tryGetInterface<IExitGameInterface>()) {
				exitGameInterface->exitGame();
			} else {
				getAPI().core->quit();
			}
		}

		for (auto& e: networkFamily) {
			e.network.dataInterpolatorSet.update(t, world);
			e.network.byteDataInterpolatorSet.update(t, world);
		}
	}

private:

	ListenerSetToken sessionChangeToken;
	bool requestedExit = false;

	void updateSession()
	{
		if (getSessionService().isMultiplayer()) {
			getSessionService().getMultiplayerSession().getEntityNetworkSession()->setWorld(getWorld(), getMessageBridge());

			setupCheats();
		} else {
			clearCheats();
		}
	}

	void setupCheats() const
	{
		auto& consoleCommands = getDevService().getConsoleCommands();

		auto getQuality = UIDebugConsoleSyntax::Callback([=] () -> Vector<String>
		{
			return { "best", "average", "bad", "veryBad" };
		});
		
		consoleCommands.addCommand("networkQuality", [=](Vector<String> args) -> String
		{
			const auto level = fromString<NetworkService::Quality>(args[0]);
			getSessionService().getMultiplayerSession().setNetworkQuality(level);
			return "Set network quality level to " + args[0];
		}, { { "quality", getQuality } });

		consoleCommands.addCommand("findNetworkEntityInbound", [this](Vector<String> args) -> String
		{
			if (args.size() != 1 || !args[0].isInteger()) {
				return "Error: no or malformed network ID";
			}

			EntityNetworkId networkId(args[0].toInteger());
			String output = "";

			getSessionService().getMultiplayerSession().getEntityNetworkSession()->findEntity(networkId, true, [&](EntityId entityId, NetworkSession::PeerId peerId) {
				const auto e = getWorld().tryGetEntity(entityId);
				if (e.isValid()) {
					output += e.getName() + ", entity ID " + toString(entityId) + ", peer " + toString((int) peerId) + "\n";
				} else {
					output += "invalid entity ID " + toString(entityId) + " for network ID " + toString(networkId) + ", peer " + toString((int) peerId) + "\n";
				}
			});

			if (output.isEmpty()) {
				output += "no inbound entities found with network ID " + toString(networkId) + " for active peers\n";
			}

			return output;
		}, UIDebugConsoleSyntax());
	}

	void clearCheats()
	{
		auto& consoleCommands = getDevService().getConsoleCommands();
		consoleCommands.removeCommand("networkQuality");
		consoleCommands.removeCommand("findNetworkEntityInbound");
	}
};

REGISTER_SYSTEM(NetworkReceiveSystem)
