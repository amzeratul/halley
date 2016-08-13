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
		ConnectionStatus getStatus() const override { return status; }
		void send(NetworkPacket&& packet) override;
		bool receive(NetworkPacket& packet) override;
		
		bool matchesEndpoint(const UDPEndpoint& remoteEndpoint) const;
		void onReceive(const char* data, size_t size);
		void setError(const std::string& cs);
		void onClosed();

	private:
		UDPSocket& socket;
		UDPEndpoint remote;
		ConnectionStatus status;

		// TODO: replace these with more efficient structures
		std::list<NetworkPacket> pendingSend;
		std::list<NetworkPacket> pendingReceive;
		std::array<char, 2048> sendBuffer;
		std::string error;

		void sendNext();
	};
}
