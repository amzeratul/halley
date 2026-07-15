#pragma once
#include <memory>
#include "halley/text/halleystring.h"
#include "halley/support/logger.h"
#include "devcon_server.h"
#include "devcon_connection.h"
#include "halley/api/core_api.h"

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
		mutable Mutex mutex;
	};

	class IDevConContextListener {
	public:
		virtual ~IDevConContextListener() = default;
		virtual void onContext(const String& context, bool updateOnly) = 0;
	};

	class DevConClient : public DevConConnection, private ILoggerSink, private CoreAPI::IProfileCallback
	{
		friend class DevConClientonnection;
		friend class DevConInterest;

	public:
		DevConClient(const HalleyAPI& api, Resources& resources, std::unique_ptr<NetworkService> service);
		~DevConClient() override;

		void connect(const String& deviceName, const ConfigNode& clientParams, const String& address, int port = DevCon::devConPort);
		void update(Time t) override;

		DevConInterest& getInterest() const;

		void setI18N(I18N* i18n);
		void setContextListener(IDevConContextListener* listener);

		uint32_t getPushSaveFileVersion() const;

	protected:
		void onReceiveMessage(const DevCon::ReloadAssetsMsg& msg) override;
		void onReceiveMessage(DevCon::RegisterInterestMsg& msg) override;
		void onReceiveMessage(DevCon::UpdateInterestMsg& msg) override;
		void onReceiveMessage(const DevCon::UnregisterInterestMsg& msg) override;
		void onReceiveMessage(DevCon::UpdateStringsMsg& msg) override;
		void onReceiveMessage(DevCon::OpenContextMsg& msg) override;

		void notifyInterest(uint32_t handle, ConfigNode data);

		void onProfileData(std::shared_ptr<ProfilerData> data) override;

	private:
		const HalleyAPI& api;
		Resources& resources;
		std::unique_ptr<NetworkService> service;

		std::unique_ptr<DevConInterest> interest;

		bool receivingProfilerData = false;

		I18N* i18n = nullptr;
		Vector<DevCon::UpdateStringsMsg> pendingUpdateStringMessages;

		IDevConContextListener* contextListener = nullptr;

		std::atomic<uint32_t> pushSaveFileVersion;

		void log(LoggerLevel level, const std::string_view msg) override;
		void applyUpdateStrings(DevCon::UpdateStringsMsg& msg);
		void updateI18NInterest();
	};
}
