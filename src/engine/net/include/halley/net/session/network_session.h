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
		using PeerId = uint8_t;

		class Listener {
		public:
			virtual ~Listener() = default;
			virtual void onStartSession(PeerId myPeerId) {}
			virtual void onPeerConnected(PeerId peerId) {}
			virtual void onPeerDisconnected(PeerId peerId) {}
		};

		NetworkSession(NetworkService& service);
		virtual ~NetworkSession();

		void update();

		void host(uint16_t maxClients);
		void join(const String& address);
		void close();

		void setMaxClients(uint16_t clients);
		uint16_t getMaxClients() const;

		std::optional<PeerId> getMyPeerId() const;
		uint16_t getClientCount() const;

		ConnectionStatus getStatus() const;
		NetworkSessionType getType() const;

		void sendToPeers(OutboundNetworkPacket packet, std::optional<PeerId> except = {});
		void sendToPeer(OutboundNetworkPacket packet, PeerId peerId);
		bool receive(InboundNetworkPacket& packet);

		void addListener(Listener* listener);
		void removeListener(Listener* listener);

	protected:
		SharedData& doGetMySharedData();
		SharedData& doGetMutableSessionSharedData();
		const SharedData& doGetSessionSharedData() const;
		const SharedData& doGetClientSharedData(PeerId clientId) const;
		const SharedData* doTryGetClientSharedData(PeerId clientId) const;

		virtual std::unique_ptr<SharedData> makeSessionSharedData();
		virtual std::unique_ptr<SharedData> makePeerSharedData();

	private:
		struct Peer {
			PeerId peerId = -1;
			std::shared_ptr<IConnection> connection;
		};
		
		NetworkService& service;
		NetworkSessionType type = NetworkSessionType::Undefined;

		uint16_t maxClients = 0;
		std::optional<PeerId> myPeerId;

		std::unique_ptr<SharedData> sessionSharedData;
		std::map<int, std::unique_ptr<SharedData>> sharedData;

		std::vector<Peer> peers;
		std::vector<InboundNetworkPacket> inbox;

		std::vector<Listener*> listeners;

		OutboundNetworkPacket makeOutbound(gsl::span<const gsl::byte> data, NetworkSessionMessageHeader header);
		void doSendToAll(OutboundNetworkPacket packet, std::optional<PeerId> except);
		
		void closeConnection(PeerId peerId, const String& reason);
		void processReceive();

		void retransmitControlMessage(PeerId peerId, gsl::span<const gsl::byte> bytes);
		void receiveControlMessage(PeerId peerId, InboundNetworkPacket& packet);
		void onControlMessage(PeerId peerId, const ControlMsgSetPeerId& msg);
		void onControlMessage(PeerId peerId, const ControlMsgSetPeerState& msg);
		void onControlMessage(PeerId peerId, const ControlMsgSetSessionState& msg);

		void setMyPeerId(PeerId id);

		void checkForOutboundStateChanges(std::optional<PeerId> ownerId);
		OutboundNetworkPacket makeUpdateSharedDataPacket(std::optional<PeerId> ownerId);
		
		OutboundNetworkPacket doMakeControlPacket(NetworkSessionControlMessageType msgType, OutboundNetworkPacket packet);

		void onConnection(NetworkService::Acceptor& acceptor);
		void acceptConnection(std::shared_ptr<IConnection> move);
		std::optional<PeerId> allocatePeerId() const;

		void disconnectPeer(Peer& peer);
	};
}
