#pragma once

#include "halley/data_structures/vector.h"
#include <memory>
#include "halley/text/halleystring.h"
#include <set>
#include <gsl/span>

#include "halley/data_structures/config_node.h"
#include "halley/time/halleytime.h"

namespace Halley
{
	class NetworkService;
	class IConnection;
	class MessageQueue;

	namespace DevCon {
		constexpr static int devConPort = 12500;
		class LogMsg;
		class ReloadAssetsMsg;
	}

	class DevConServerConnection
	{
	public:
		DevConServerConnection(std::shared_ptr<IConnection> connection);
		
		void update(Time t);
		
		void reloadAssets(gsl::span<const String> assetIds);

		void registerInterest(const String& id, const ConfigNode& params, uint32_t handle);
		void unregisterInterest(uint32_t handle);

	private:
		std::shared_ptr<IConnection> connection;
		std::shared_ptr<MessageQueue> queue;

		void onReceiveLogMsg(const DevCon::LogMsg& msg);
	};

	class DevConServer
	{
	public:
		using InterestCallback = std::function<void(ConfigNode)>;
		using InterestHandle = uint32_t;

		DevConServer(std::unique_ptr<NetworkService> service, int port = DevCon::devConPort);

		void update(Time t);

		void reloadAssets(gsl::span<const String> assetIds);

		InterestHandle registerInterest(String id, ConfigNode params, InterestCallback callback);
		void unregisterInterest(InterestHandle handle);

	private:
		struct Interest {
			String id;
			ConfigNode config;
			InterestCallback callback;
		};

		std::unique_ptr<NetworkService> service;
		Vector<std::shared_ptr<DevConServerConnection>> connections;

		HashMap<InterestHandle, Interest> interest;
		uint32_t interestId = 0;

		void initConnection(DevConServerConnection& conn);
	};
}
