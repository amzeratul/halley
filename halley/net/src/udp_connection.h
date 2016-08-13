#pragma once

#include "iconnection.h"
#include <boost/asio.hpp>

namespace Halley
{
	class NetworkService;
	using UDPEndpoint = boost::asio::ip::udp::endpoint;
	using UDPSocket = boost::asio::ip::udp::socket;

	class UDPConnection : public IConnection
	{
	public:
		UDPConnection(UDPSocket& socket, UDPEndpoint remote);
		void close() override;
		bool isOpen() const override;
		void send(NetworkPacket&& packet) override;
		bool receive(NetworkPacket& packet) override;
		
		bool matchesEndpoint(const UDPEndpoint& remoteEndpoint) const;
		void onReceive(const char* data, size_t size);
		void setError(const std::string& cs);

	private:
		UDPSocket& socket;
		UDPEndpoint remote;
		bool open;

		// TODO: replace these with more efficient structures
		std::list<NetworkPacket> pendingSend;
		std::list<NetworkPacket> pendingReceive;
		std::array<char, 2048> sendBuffer;
		std::string error;

		void sendNext();
	};
}
