#pragma once
#include <memory>
#include "halley/text/halleystring.h"
#include "halley/support/logger.h"
#include "devcon_server.h"

namespace Halley
{
	class DevConClient;

	namespace DevCon {
		class RPCMsg;
		class UpdateInterestMsg;
		class UnregisterInterestMsg;
		class RegisterInterestMsg;
	}

	class NetworkService;
	class HalleyAPI;
	class IConnection;
	class MessageQueue;
	class Resources;

	class DevConInterest {
	public:
		DevConInterest(DevConClient& parent);
		
		void registerInterest(String id, ConfigNode config, uint32_t handle);
		void updateInterest(uint32_t handle, ConfigNode config);
		void unregisterInterest(uint32_t handle);

		bool hasInterest(const String& id) const;
		gsl::span<const ConfigNode> getInterestConfigs(const String& id) const;
		void notifyInterest(const String& id, size_t configIdx, ConfigNode data);

	private:
		DevConClient& parent;

		struct InterestGroup {
			Vector<ConfigNode> configs;
			Vector<uint32_t> handles;
			Vector<ConfigNode> lastResults;
		};

		HashMap<String, InterestGroup> interests;
	};

	class DevConClient : private ILoggerSink
	{
		friend class DevConClientonnection;
		friend class DevConInterest;

	public:
		using RPCHandle = std::function<Future<ConfigNode>(ConfigNode)>;

		DevConClient(const HalleyAPI& api, Resources& resources, std::unique_ptr<NetworkService> service);
		~DevConClient() override;

		void connect(const String& deviceName, const ConfigNode& clientParams, const String& address, int port = DevCon::devConPort);
		void update(Time t);

		DevConInterest& getInterest() const;

		void setRPCHandle(const String& method, RPCHandle handle);
		void setDebugConsoleController(std::shared_ptr<UIDebugConsoleController> consoleController);

	protected:
		void onReceiveMessage(const DevCon::ReloadAssetsMsg& msg);
		void onReceiveMessage(DevCon::RegisterInterestMsg& msg);
		void onReceiveMessage(DevCon::UpdateInterestMsg& msg);
		void onReceiveMessage(const DevCon::UnregisterInterestMsg& msg);
		void onReceiveMessage(DevCon::RPCMsg& msg);

		void notifyInterest(uint32_t handle, ConfigNode data);

	private:
		const HalleyAPI& api;
		Resources& resources;
		std::unique_ptr<NetworkService> service;

		std::shared_ptr<MessageQueue> queue;

		std::unique_ptr<DevConInterest> interest;

		HashMap<String, RPCHandle> rpcHandles;

		void log(LoggerLevel level, const std::string_view msg) override;
	};
}
