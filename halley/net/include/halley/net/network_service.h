#pragma once
#include <memory>
#include <vector>
#include <halley/text/halleystring.h>
#include "iconnection.h"

namespace Halley
{
	class NetworkServicePImpl;

	class NetworkService
	{
	public:
		NetworkService(int port);
		~NetworkService();

		void update();

		void setAcceptingConnections(bool accepting);
		IConnection* tryAcceptConnection();
		IConnection* connect(String address, int port);

	private:
		std::unique_ptr<NetworkServicePImpl> pimpl;
		std::vector<std::unique_ptr<IConnection>> activeConnections;
		bool acceptingConnections = false;
		bool startedListening = false;

		void startListening();
		void receiveNext();
	};
}
