#pragma once
#include <memory>
#include "halley/text/halleystring.h"
#include "halley/support/logger.h"
#include "devcon_server.h"

namespace Halley
{
	class NetworkService;
	class HalleyAPI;
	class IConnection;
	class MessageQueue;

	class DevConClient : private ILoggerSink
	{
	public:
		DevConClient(const HalleyAPI& api, std::unique_ptr<NetworkService> service, const String& address, int port = DevCon::devConPort);
		~DevConClient();

		void update();

		void onReceiveReloadAssets(const DevCon::ReloadAssetsMsg& msg);

	private:
		const HalleyAPI& api;
		std::unique_ptr<NetworkService> service;
		String address;
		int port;

		std::shared_ptr<IConnection> connection;
		std::shared_ptr<MessageQueue> queue;

		void connect();
		void log(LoggerLevel level, const String& msg) override;
	};
}
