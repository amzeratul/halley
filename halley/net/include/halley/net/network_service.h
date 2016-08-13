#pragma once
#include <memory>
#include <vector>
#include <halley/text/halleystring.h>
#include "udp_connection.h"

namespace Halley
{
	class NetworkService
	{
	public:
		NetworkService();
		~NetworkService();

		void update();

		void startListening(int port);
		void stopListening();

		std::shared_ptr<UDPConnection> tryAcceptConnection();
		std::shared_ptr<UDPConnection> connect(String address, int port);

	private:
		void closePendingConnections();
		void closeActiveConnections();
	};
}
