#pragma once
#include <memory>
#include "halley/text/halleystring.h"
#include "halley/support/logger.h"
#include "devcon_server.h"

namespace Halley
{
	class DevConClient;

	namespace DevCon {
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
		DevConClient(const HalleyAPI& api, Resources& resources, std::unique_ptr<NetworkService> service, String address, int port = DevCon::devConPort);
		~DevConClient();

		void update(Time t);

		DevConInterest& getInterest() const;

	protected:
		void onReceiveReloadAssets(const DevCon::ReloadAssetsMsg& msg);
		void onReceiveRegisterInterest(DevCon::RegisterInterestMsg& msg);
		void onReceiveUnregisterInterest(const DevCon::UnregisterInterestMsg& msg);
		void notifyInterest(uint32_t handle, ConfigNode data);

	private:
		const HalleyAPI& api;
		Resources& resources;
		std::unique_ptr<NetworkService> service;
		String address;
		int port;

		std::shared_ptr<MessageQueue> queue;

		std::unique_ptr<DevConInterest> interest;

		void connect();
		void log(LoggerLevel level, const String& msg) override;
	};
}
