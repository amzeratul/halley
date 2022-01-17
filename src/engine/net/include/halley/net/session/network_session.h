#pragma once
#include "halley/utils/utils.h"
#include "halley/bytes/byte_serializer.h"
#include "../connection/iconnection.h"
#include "../connection/network_packet.h"
#include "network_session_messages.h"
#include "shared_data.h"
#include "network_session_control_messages.h"
#include "../connection/network_service.h"

namespace Halley {
	class NetworkService;

	class NetworkSession {
	public:
		NetworkSession(NetworkService& service);
		virtual ~NetworkSession();

		void host(uint16_t maxClients);
		void join(const String& address);

		void setMaxClients(uint16_t clients);
		uint16_t getMaxClients() const;

		int getMyPeerId() const;

		uint16_t getClientCount() const;
		void acceptConnection(std::shared_ptr<IConnection> move);
		void update();

		NetworkSessionType getType() const;

		void close(); // Called from destructor, hence final
		ConnectionStatus getStatus() const;
		void send(OutboundNetworkPacket packet);
		bool receive(InboundNetworkPacket& packet);

	protected:
		SharedData& doGetMySharedData();
		SharedData& doGetMutableSessionSharedData();
		const SharedData& doGetSessionSharedData() const;
		const SharedData& doGetClientSharedData(int clientId) const;
		const SharedData* doTryGetClientSharedData(int clientId) const;

		virtual std::unique_ptr<SharedData> makeSessionSharedData() = 0;
		virtual std::unique_ptr<SharedData> makePeerSharedData() = 0;

		virtual void onStartSession();
		virtual void onPeerIdAssigned();
		virtual void onHosting();
		virtual void onConnected(int peerId);
		virtual void onDisconnected(int peerId);
		
	private:
		NetworkService& service;
		NetworkSessionType type = NetworkSessionType::Undefined;

		uint16_t maxClients = 0;
		int myPeerId = -1;

		std::unique_ptr<SharedData> sessionSharedData;
		std::map<int, std::unique_ptr<SharedData>> sharedData;

		std::vector<std::shared_ptr<IConnection>> connections;
		std::vector<InboundNetworkPacket> inbox;

		OutboundNetworkPacket makeOutbound(gsl::span<const gsl::byte> data, NetworkSessionMessageHeader header);
		void sendToAll(OutboundNetworkPacket packet, int except = -1);
		void closeConnection(int peerId, const String& reason);
		void processReceive();

		void retransmitControlMessage(int peerId, gsl::span<const gsl::byte> bytes);
		void receiveControlMessage(int peerId, InboundNetworkPacket& packet);
		void onControlMessage(int peerId, const ControlMsgSetPeerId& msg);
		void onControlMessage(int peerId, const ControlMsgSetPeerState& msg);
		void onControlMessage(int peerId, const ControlMsgSetSessionState& msg);

		void setMyPeerId(int id);

		void checkForOutboundStateChanges(int ownerId);
		OutboundNetworkPacket makeUpdateSharedDataPacket(int ownerId);
		
		OutboundNetworkPacket doMakeControlPacket(NetworkSessionControlMessageType msgType, OutboundNetworkPacket packet);

		void onConnection(NetworkService::Acceptor& acceptor);
	};

	template <typename SessionSharedDataType, typename PeerSharedDataType>
	class NetworkSessionImpl : public NetworkSession {
	public:
		NetworkSessionImpl(NetworkService& s) : NetworkSession(s) {}
		virtual ~NetworkSessionImpl() {}

		SessionSharedDataType& getMutableSessionSharedData()
		{
			return static_cast<SessionSharedDataType&>(doGetMutableSessionSharedData());
		}
		
		const SessionSharedDataType& getSessionSharedData() const
		{
			return static_cast<const SessionSharedDataType&>(doGetSessionSharedData());
		}

		PeerSharedDataType& getMySharedData()
		{
			return static_cast<PeerSharedDataType&>(doGetMySharedData());
		}

		const PeerSharedDataType& getClientSharedData(int clientId) const
		{
			return static_cast<const PeerSharedDataType&>(doGetClientSharedData(clientId));
		}

		const PeerSharedDataType* tryGetClientSharedData(int clientId) const
		{
			return static_cast<const PeerSharedDataType*>(doTryGetClientSharedData(clientId));
		}

	protected:
		std::unique_ptr<SharedData> makeSessionSharedData() override final
		{
			return std::make_unique<SessionSharedDataType>();
		}
		
		std::unique_ptr<SharedData> makePeerSharedData() override final
		{
			return std::make_unique<PeerSharedDataType>();
		}
	};
}
