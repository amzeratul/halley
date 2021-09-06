#pragma once

#include <vector>
#include <memory>
#include "halley/text/halleystring.h"
#include <set>
#include <gsl/span>

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
		
		void update();
		
		void reloadAssets(gsl::span<const String> assetIds);

	private:
		std::shared_ptr<IConnection> connection;
		std::shared_ptr<MessageQueue> queue;

		void onReceiveLogMsg(const DevCon::LogMsg& msg);
	};

	class DevConServer
	{
	public:
		DevConServer(std::unique_ptr<NetworkService> service, int port = DevCon::devConPort);

		void update();

		void reloadAssets(gsl::span<const String> assetIds);

	private:
		std::unique_ptr<NetworkService> service;
		std::vector<std::shared_ptr<DevConServerConnection>> connections;
	};
}
