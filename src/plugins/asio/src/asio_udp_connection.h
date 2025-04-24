#pragma once

#include "halley/net/connection/iconnection.h"
#include "halley/net/connection/network_packet.h"

#ifdef _MSC_VER
#pragma warning(disable: 4834)
#endif

#include <asio.hpp>

namespace Halley
{
	using UDPEndpoint = asio::ip::udp::endpoint;
	using UDPSocket = asio::ip::udp::socket;

	class AsioUDPConnection : public IConnection
	{
	public:
		explicit AsioUDPConnection(UDPSocket& socket, UDPEndpoint remote);

		void close() override;
		ConnectionStatus getStatus() const override { return status; }
		bool isSupported(TransmissionType type) const override;
		void send(TransmissionType type, OutboundNetworkPacket packet) override;
		bool receive(InboundNetworkPacket& packet) override;
		
		bool matchesEndpoint(const UDPEndpoint& remoteEndpoint) const;
		void setError(const std::string& cs);

		void terminateConnection();
		short getConnectionId() const { return connectionId; }

		String getRemoteAddress() const override;

        [[nodiscard]] size_t getMaxUnreliablePacketSize() const override;

        void onConnect(short connId) override;

        void sendUnreliablePacket(gsl::span<const gsl::byte> packet) override;
        void setUnreliablePacketListener(IPacketListener* listener) override;

        static void receiveAll(
                UDPSocket &socket, HashMap<short, std::shared_ptr<AsioUDPConnection>>& connections,
                const std::function<void(UDPEndpoint& remote, gsl::span<gsl::byte> packet)>& unknownConnCallback);

	private:
		UDPSocket& socket;
		UDPEndpoint remote;
		ConnectionStatus status;
		short connectionId;

		std::string error;

        IPacketListener* packetListener = nullptr;
	};
}
