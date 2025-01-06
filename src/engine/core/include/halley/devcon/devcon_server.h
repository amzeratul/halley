#pragma once

#include "halley/data_structures/vector.h"
#include <memory>
#include "halley/text/halleystring.h"
#include <set>
#include <gsl/span>

#include "halley/data_structures/config_node.h"
#include "halley/time/halleytime.h"
#include "halley/ui/widgets/ui_debug_console.h"
#include "devcon_connection.h"

namespace Halley
{
	class IProject;
	class NetworkService;
	class IConnection;
	class MessageQueue;

	namespace DevCon {
		class RPCReplyMsg;
		class SetClientDataMsg;
		constexpr static int devConPort = 12500;
		class LogMsg;
		class ReloadAssetsMsg;
		class NotifyInterestMsg;
	}

	class DevConServer;

	class DevConClientInfo {
	public:
		GamePlatform platform;
		String deviceName;
		ConfigNode params;
	};

	class DevConServerConnection : public DevConConnection
	{
		friend class DevConServer;

	public:
		DevConServerConnection(DevConServer& parent, size_t id, std::shared_ptr<IConnection> connection);
		
		size_t getId() const;
		const std::optional<DevConClientInfo>& getClientInfo() const;

		DevConServer& getParent();
	
		void reloadAssets(Vector<String> assetIds, Vector<String> packIds);

		Vector<DevCon::LogMsg> movePendingLogs();

	private:
		DevConServer& parent;
		size_t id;

		std::optional<DevConClientInfo> clientInfo;

		Vector<DevCon::LogMsg> pendingLogs;

		void registerInterest(const String& id, const ConfigNode& params, uint32_t handle);
		void updateInterest(uint32_t handle, const ConfigNode& params);
		void unregisterInterest(uint32_t handle);

		void onReceiveMessage(DevCon::LogMsg& msg) override;
		void onReceiveMessage(DevCon::NotifyInterestMsg& msg) override;
		void onReceiveMessage(DevCon::SetClientDataMsg& msg) override;
	};

	class DevConServer
	{
		friend class DevConServerConnection;

	public:
		using InterestCallback = std::function<void(size_t, ConfigNode)>;
		using InterestHandle = uint32_t;

		DevConServer(std::unique_ptr<NetworkService> service, int port = DevCon::devConPort);
		
		void update(Time t);

		void reloadAssets(Vector<String> assetIds, Vector<String> packIds);

		InterestHandle registerInterest(String id, ConfigNode params, InterestCallback callback, std::optional<size_t> connectionId = {});
		void updateInterest(InterestHandle handle, ConfigNode params);
		void unregisterInterest(InterestHandle handle);
		const ConfigNode& getInterestParams(InterestHandle handle) const;

		gsl::span<std::shared_ptr<DevConServerConnection>> getConnections();

		void setProject(IProject* project);

	protected:
		void onReceiveNotifyInterestMsg(const DevConServerConnection& connection, DevCon::NotifyInterestMsg& msg);

	private:
		struct Interest {
			String id;
			ConfigNode config;
			InterestCallback callback;
			Vector<size_t> hadResult;
		};

		std::unique_ptr<NetworkService> service;
		Vector<std::shared_ptr<DevConServerConnection>> connections;
		size_t connId = 0;

		HashMap<InterestHandle, Interest> interest;
		InterestHandle interestId = 0;

		IProject* project = nullptr;

		void initConnection(DevConServerConnection& conn);
		void terminateConnection(DevConServerConnection& conn);
	};
}
