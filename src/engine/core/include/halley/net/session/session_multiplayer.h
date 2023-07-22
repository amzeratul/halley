#pragma once

#include "session.h"
#include "halley/api/platform_api.h"
#include "halley/file_formats/config_file.h"
#include "halley/net/entity/entity_network_session.h"

namespace Halley {
	class HalleyAPI;

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
		bool isReadyToStart() const override;
		bool hasHostAuthority() const override;
		Vector<Rect4f> getRemoteViewPorts() const override;

		bool isWaitingForInitialViewPort() const override;
		void reportInitialViewPort(Rect4f viewPort) override;

		EntityNetworkSession* getEntityNetworkSession() override;
		NetworkSession* getNetworkSession() override;

		const String& getPlayerName() const override;

		void setNetworkQuality(NetworkService::Quality level);

	protected:
		void onStartSession(NetworkSession::PeerId myPeerId) override;
		void onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId) override;
		void setupInterpolators(DataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote) override;
		bool isEntityInView(EntityRef entity, const EntityClientSharedData& clientData) override;

	private:
		bool host = false;
		bool waitingForViewPort = false;
		String playerName;

		std::unique_ptr<EntityNetworkSession> entitySession;
		std::shared_ptr<NetworkSession> session;
		std::shared_ptr<NetworkService> service;
		std::unique_ptr<MultiplayerLobby> lobby;

		void setupDictionary(SerializationDictionary& dict, std::shared_ptr<const ConfigFile> config);
	};
}
