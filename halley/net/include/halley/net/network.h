#pragma once
#include <memory>
#include <vector>
#include <halley/text/halleystring.h>
#include "udp_connection.h"

namespace Halley
{
	class Network
	{
	public:
		Network();
		~Network();

		void update();

		void startListening(int port);
		void stopListening();

		std::shared_ptr<UDPConnection> tryAcceptConnection();
		std::shared_ptr<UDPConnection> openConnection(String address, int port);

	private:
		void closePendingConnections();
		void closeActiveConnections();
		
		int listeningPort = -1;
		std::vector<std::shared_ptr<UDPConnection>> pending;
		std::vector<std::shared_ptr<UDPConnection>> active;
	};
}
