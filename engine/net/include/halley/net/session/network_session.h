#pragma once
#include "halley/utils/utils.h"
#include "halley/file/byte_serializer.h"
#include "../connection/iconnection.h"
#include "../connection/network_packet.h"
#include "network_session_messages.h"
#include "shared_data.h"
#include "network_session_control_messages.h"

namespace Halley {
	class NetworkService;

	class NetworkSession : public IConnection {
	public:
		NetworkSession(NetworkService& service);
		virtual ~NetworkSession();

		void host(int port);
		void join(const String& address, int port);

		void setMaxClients(int clients);
		int getClientCount() const;
		void acceptConnection(std::shared_ptr<IConnection> move);
		void update();

		NetworkSessionType getType() const;

		void close() final override; // Called from destructor, hence final
		ConnectionStatus getStatus() const override;
		void send(OutboundNetworkPacket&& packet) override;
		bool receive(InboundNetworkPacket& packet) override;

	protected:
		SharedData& doGetMySharedData();
		SharedData& doGetMutableSessionSharedData();
		const SharedData& doGetSessionSharedData() const;
		const SharedData& doGetClientSharedData(int clientId) const;

		virtual std::unique_ptr<SharedData> makeSessionSharedData() = 0;
		virtual std::unique_ptr<SharedData> makePeerSharedData() = 0;
		
	private:
		NetworkService& service;
		NetworkSessionType type = NetworkSessionType::Undefined;

		int maxClients = 0;
		int myPeerId = -1;

		std::unique_ptr<SharedData> sessionSharedData;
		std::map<int, std::unique_ptr<SharedData>> sharedData;

		std::vector<std::shared_ptr<IConnection>> connections;
		std::vector<InboundNetworkPacket> inbox;

		OutboundNetworkPacket makeOutbound(gsl::span<const gsl::byte> data, NetworkSessionMessageHeader header);
		void sendToAll(OutboundNetworkPacket&& packet, int except = -1);
		void closeConnection(int peerId, const String& reason);
		void processReceive();

		void retransmitControlMessage(int peerId, gsl::span<const gsl::byte> bytes);
		void receiveControlMessage(int peerId, InboundNetworkPacket& packet);
		void onControlMessage(int peerId, const ControlMsgSetPeerId& msg);
		void onControlMessage(int peerId, const ControlMsgSetPeerState& msg);
		void onControlMessage(int peerId, const ControlMsgSetSessionState& msg);

		void setMyPeerId(int id);

		void checkForOutboundStateChanges(bool isSessionState, SharedData& data);

		template <typename T>
		OutboundNetworkPacket makeControlPacket(NetworkSessionControlMessageType msgType, const T& data)
		{
			return doMakeControlPacket(msgType, OutboundNetworkPacket(data));
		}

		OutboundNetworkPacket doMakeControlPacket(NetworkSessionControlMessageType msgType, OutboundNetworkPacket&& packet);
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
