#pragma once
#include <memory>
#include <vector>
#include <halley/text/halleystring.h>
#include "iconnection.h"
#include <boost/asio/detail/addressof.hpp>
#include "../../../src/udp_connection.h"

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
		bool acceptingConnections = false;
		bool startedListening = false;

		void startListening();
		void receiveNext();
		void onNewConnectionRequest(char* data, size_t size, const UDPEndpoint& remoteEndpoint);
	};
}
