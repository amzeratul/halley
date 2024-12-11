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
	class AckUnreliableConnectionStats;
	class MessageQueueUDP;
	class NetworkService;

	class NetworkSession {
	public:
		using PeerId = uint8_t;

		class IListener {
		public:
			virtual ~IListener() = default;
			virtual void onStartSession(PeerId myPeerId) {}
			virtual void onPeerConnected(PeerId peerId) {}
			virtual void onPeerDisconnected(PeerId peerId) {}
		};

		class ISharedDataHandler {
		public:
			virtual ~ISharedDataHandler() = default;
			virtual std::unique_ptr<SharedData> makeSessionSharedData() { return {}; }
			virtual std::unique_ptr<SharedData> makePeerSharedData() { return {}; }
		};

		class IServerSideDataHandler {
		public:
			virtual ~IServerSideDataHandler() = default;
			virtual bool setServerSideData(String uniqueKey, ConfigNode data) = 0;
			virtual ConfigNode getServerSideData(String uniqueKey) = 0;
		};

		NetworkSession(NetworkService& service, uint32_t networkVersion, String userName, ISharedDataHandler* sharedDataHandler = nullptr);
		virtual ~NetworkSession();

		void update(Time t);

		void host(uint16_t maxClients);
		void join(const String& address);
		void close();

		void setMaxClients(uint16_t clients);
		uint16_t getMaxClients() const;

		std::optional<PeerId> getMyPeerId() const;
		uint16_t getClientCount() const;
		Vector<PeerId> getRemotePeers() const;

		ConnectionStatus getStatus() const;
		NetworkSessionType getType() const;

		void sendToPeers(OutboundNetworkPacket packet, std::optional<PeerId> except = {});
		void sendToPeer(OutboundNetworkPacket packet, PeerId peerId);
		std::optional<std::pair<PeerId, InboundNetworkPacket>> receive();

		void addListener(IListener* listener);
		void removeListener(IListener* listener);
		void setSharedDataHandler(ISharedDataHandler* sharedDataHandler);
		void setServerSideDataHandler(IServerSideDataHandler* serverSideDataHandler);

		const String& getHostAddress() const;
		NetworkService& getService() const;

		size_t getNumConnections() const;
		const AckUnreliableConnectionStats& getConnectionStats(size_t idx) const;
		float getLatency(size_t idx) const;

		template <typename T>
		T& getMySharedData()
		{
			return dynamic_cast<T&>(doGetMySharedData());
		}
		
		template <typename T>
		T& getMutableSessionSharedData()
		{
			return dynamic_cast<T&>(doGetMutableSessionSharedData());
		}

		template <typename T>
		const T& getSessionSharedData() const
		{
			return dynamic_cast<const T&>(doGetSessionSharedData());
		}

		template <typename T>
		const T& getClientSharedData(PeerId clientId) const
		{
			return dynamic_cast<const T&>(doGetClientSharedData(clientId));
		}

		template <typename T>
		const T* tryGetClientSharedData(PeerId clientId) const
		{
			return dynamic_cast<const T*>(doTryGetClientSharedData(clientId));
		}

		bool hasSessionSharedData() const;

		Future<bool> setServerSideData(String uniqueKey, ConfigNode data);
		Future<ConfigNode> retrieveServerSideData(String uniqueKey);

	protected:
		SharedData& doGetMySharedData();
		SharedData& doGetMutableSessionSharedData();
		const SharedData& doGetSessionSharedData() const;
		const SharedData& doGetClientSharedData(PeerId clientId) const;
		const SharedData* doTryGetClientSharedData(PeerId clientId) const;

		std::unique_ptr<SharedData> makeSessionSharedData();
		std::unique_ptr<SharedData> makePeerSharedData();

	private:
		struct Peer {
			PeerId peerId = -1;
			bool alive = true;
			std::shared_ptr<MessageQueueUDP> connection;
			std::shared_ptr<AckUnreliableConnectionStats> stats;

			ConnectionStatus getStatus() const;
		};
		
		NetworkService& service;
		NetworkSessionType type = NetworkSessionType::Undefined;
		String hostAddress;
		ISharedDataHandler* sharedDataHandler = nullptr;
		IServerSideDataHandler* serverSideDataHandler = nullptr;

		uint32_t networkVersion;
		String userName;

		uint16_t maxClients = 0;
		std::optional<PeerId> myPeerId;

		std::unique_ptr<SharedData> sessionSharedData;
		std::map<int, std::unique_ptr<SharedData>> sharedData;

		Vector<Peer> peers;
		Vector<std::pair<PeerId, InboundNetworkPacket>> inbox;

		Vector<IListener*> listeners;

		uint32_t requestId = 0;
		HashMap<uint32_t, Promise<bool>> setServerSideDataPending;
		HashMap<uint32_t, Promise<ConfigNode>> getServerSideDataPending;

		OutboundNetworkPacket makeOutbound(gsl::span<const gsl::byte> data, NetworkSessionMessageHeader header);
		void doSendToAll(OutboundNetworkPacket packet, std::optional<PeerId> except);
		void doSendToPeer(const Peer& peer, OutboundNetworkPacket packet);
		
		void closeConnection(PeerId peerId, const String& reason);
		void processReceive();

		void retransmitControlMessage(PeerId peerId, gsl::span<const gsl::byte> bytes);
		void receiveControlMessage(PeerId peerId, InboundNetworkPacket& packet);
		void onControlMessage(PeerId peerId, const ControlMsgJoin& msg);
		void onControlMessage(PeerId peerId, const ControlMsgSetPeerId& msg);
		void onControlMessage(PeerId peerId, const ControlMsgSetPeerState& msg);
		void onControlMessage(PeerId peerId, const ControlMsgSetSessionState& msg);
		void onControlMessage(PeerId peerId, const ControlMsgSetServerSideData& msg);
		void onControlMessage(PeerId peerId, const ControlMsgSetServerSideDataReply& msg);
		void onControlMessage(PeerId peerId, const ControlMsgGetServerSideData& msg);
		void onControlMessage(PeerId peerId, const ControlMsgGetServerSideDataReply& msg);

		void setMyPeerId(PeerId id);
		Peer& getPeer(PeerId id);

		void checkForOutboundStateChanges(Time t, std::optional<PeerId> ownerId);
		OutboundNetworkPacket makeUpdateSharedDataPacket(std::optional<PeerId> ownerId);
		
		OutboundNetworkPacket doMakeControlPacket(NetworkSessionControlMessageType msgType, OutboundNetworkPacket packet);

		void onConnection(NetworkService::Acceptor& acceptor);
		void acceptConnection(std::shared_ptr<IConnection> move);
		std::optional<PeerId> allocatePeerId() const;

		void disconnectPeer(Peer& peer);

		Peer makePeer(PeerId peerId, std::shared_ptr<IConnection> connection);

		bool doSetServerSideData(String uniqueKey, ConfigNode data);
		ConfigNode doGetServerSideData(String uniqueKey);
	};
}
