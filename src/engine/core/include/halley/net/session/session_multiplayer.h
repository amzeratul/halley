#pragma once

#include "session.h"
#include "halley/api/platform_api.h"
#include "halley/file_formats/config_file.h"
#include "halley/net/entity/entity_network_session.h"

namespace Halley {
	class HalleyAPI;

	enum class SessionState {
		Idle,
		WaitingForPlatformLobbyCallback, // Waiting for platform e.g. Steam to give lobby info
		JoiningSession, // Connecting, waiting for host to assign peer id
		GameLobbyReady, // Peer id assigned, ready to join in-game lobby
		JoiningGame, // Starting game
		WaitingForInitialViewport, // Host is waiting for initial viewport
		WaitingToStart, // Client is waiting for the host to confirm they're ready to go
		PlayingGame, // Playing
		Disconnected
	};

	template <>
	struct EnumNames<SessionState> {
		constexpr std::array<const char*, 9> operator()() const {
			return{ {
				"Idle",
				"WaitingForPlatformLobbyCallback",
				"JoiningSession",
				"GameLobbyReady",
				"JoiningGame",
				"WaitingForInitialViewport",
				"WaitingToStart",
				"PlayingGame",
				"Disconnected"
			} };
		}
	};

	class SessionMultiplayer : public Session, public EntityNetworkSession::IEntityNetworkSessionListener {
	public:
		enum class Mode {
			Host,
			Join,
			WaitForLobby
		};

		struct ConnectionOptions {
			Mode mode = Mode::Host;
			int16_t maxPlayers = 8;
			std::optional<String> clientConnectTo;
		};

		struct SessionSettings {
			uint32_t networkVersion;
			std::shared_ptr<const ConfigFile> serializationDict;
			std::set<String> ignoreComponents;
		};

		SessionMultiplayer(const HalleyAPI& api, Resources& resources, ConnectionOptions options, SessionSettings settings);

		bool update() override;

		bool isMultiplayer() const override;
		bool isHost() const;
		bool hasLocalSave() const override;
		bool hasHostAuthority() const override;
		Vector<Rect4f> getRemoteViewPorts() const override;
		size_t getNumberOfPlayers() const override;
		uint8_t getMyClientId() const override;
		SessionState getState() const;
		bool hasGameStarted() const;

		void reportInitialViewPort(Rect4f viewPort);
		void startOrJoinGame();

		EntityNetworkSession* getEntityNetworkSession() override;
		NetworkSession* getNetworkSession() override;

		const String& getPlayerName() const override;

		void setNetworkQuality(NetworkService::Quality level);

	protected:
		void onStartSession(NetworkSession::PeerId myPeerId) override;
		void onStartGame() override;
		void onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId) override;
		void setupInterpolators(DataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote) override;
		bool isEntityInView(EntityRef entity, const EntityClientSharedData& clientData) override;
		ConfigNode getAccountData(const ConfigNode& params) override;
		void setState(SessionState state);

	private:
		bool host = false;
		String playerName;
		SessionState curState = SessionState::Disconnected;

		std::unique_ptr<EntityNetworkSession> entitySession;
		std::shared_ptr<NetworkSession> session;
		std::shared_ptr<NetworkService> service;
		std::unique_ptr<MultiplayerLobby> lobby;

		void setupDictionary(SerializationDictionary& dict, std::shared_ptr<const ConfigFile> config);
	};
}
