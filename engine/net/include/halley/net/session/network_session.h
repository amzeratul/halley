#pragma once
#include "halley/utils/utils.h"
#include "halley/file/byte_serializer.h"
#include "../connection/iconnection.h"

namespace Halley {
	class NetworkService;

	enum class NetworkSessionType {
		Undefined,
		Host,
		Client
	};

	class NetworkSession : public IConnection {
	public:
		NetworkSession(NetworkService& service);
		~NetworkSession();

		void host(int port);
		void join(const String& address, int port);

		void setMaxClients(int clients);
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
		std::map<String, Bytes> sharedData;

		std::vector<std::shared_ptr<IConnection>> connections;
	};

    //matchSettings = session.getShared("matchSettings");
    //session.setShared("matchSettings", matchSettings);
}
