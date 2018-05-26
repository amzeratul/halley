#pragma once

#include "devcon_messages.h"
#include <vector>
#include <memory>
#include "halley/net/connection/iconnection.h"
#include "halley/net/connection/message_queue.h"

namespace Halley
{
	class NetworkService;

	class DevConServerConnection
	{
	public:
		DevConServerConnection(std::shared_ptr<IConnection> connection);
		void update();

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

	private:
		std::unique_ptr<NetworkService> service;
		std::vector<std::shared_ptr<DevConServerConnection>> connections;
	};
}
