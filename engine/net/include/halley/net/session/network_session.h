#pragma once
#include "halley/utils/utils.h"
#include "halley/file/byte_serializer.h"
#include "../connection/iconnection.h"
#include "network_session_messages.h"

namespace Halley {
	class NetworkService;

	class NetworkSession : public IConnection {
	public:
		NetworkSession(NetworkService& service);
		~NetworkSession();

		void host(int port);
		void join(const String& address, int port);

		void setMaxClients(int clients);
		int getClientCount() const;
		void update();

		NetworkSessionType getType() const;

		bool hasSharedData(const String& id) const;
		DeserializeHelper getSharedData(const String& id) const;
		void setSharedData(const String& id, Bytes&& data);

		void close() final override; // Called from destructor, hence final
		ConnectionStatus getStatus() const override;
		void send(OutboundNetworkPacket&& packet) override;
		bool receive(InboundNetworkPacket& packet) override;

	private:
		NetworkService& service;
		NetworkSessionType type = NetworkSessionType::Undefined;

		int maxClients = 0;
		int playerId = -1;

		std::map<String, Bytes> sharedData;

		std::vector<std::shared_ptr<IConnection>> connections;

		OutboundNetworkPacket makeOutbound(gsl::span<const gsl::byte> data, NetworkSessionMessageHeader header);
		void receiveControlMessage(int peerId, gsl::span<const gsl::byte> data);
		void closeConnection(int peerId, const String& reason);
	};
}
