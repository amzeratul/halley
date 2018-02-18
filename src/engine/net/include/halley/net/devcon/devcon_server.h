#pragma once

#include "devcon_messages.h"
#include <vector>
#include <memory>
#include "connection/reliable_connection.h"
#include "connection/message_queue.h"

namespace Halley
{
	class NetworkService;

	class DevConServerConnection
	{
	public:
		DevConServerConnection(std::shared_ptr<IConnection> connection);
		void update();

	private:
		std::shared_ptr<ReliableConnection> connection;
		std::shared_ptr<MessageQueue> queue;
	};

	class DevConServer
	{
	public:
		DevConServer(std::unique_ptr<NetworkService> service, int port = devConPort);

		void update();

	private:
		std::unique_ptr<NetworkService> service;
		std::vector<std::shared_ptr<DevConServerConnection>> connections;
	};
}
