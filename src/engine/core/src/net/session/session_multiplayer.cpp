#include "halley/net/session/session_multiplayer.h"

#include "halley/api/halley_api.h"
#include "halley/api/platform_api.h"
#include "halley/entity/components/transform_2d_component.h"
#include "halley/net/entity/entity_network_session.h"
#include "halley/net/session/network_session.h"

using namespace Halley;

SessionMultiplayer::SessionMultiplayer(const HalleyAPI& api, Resources& resources, ConnectionOptions options, SessionSettings settings)
	: api(api)
	, host(options.mode == Mode::Host)
{
	playerName = api.platform->getPlayerName();

	service = api.platform->createNetworkService(host ? 6060 : 0);
	if (!service) {
		throw Exception("Unable to initialize multiplayer session: platform has no network service implementation.", 0);
	}
	
	session = std::make_shared<NetworkSession>(*service, settings.networkVersion, playerName);
	entitySession = std::make_unique<EntityNetworkSession>(session, resources, std::move(settings.ignoreComponents), this);
	setupDictionary(entitySession->getSerializationDictionary(), std::move(settings.serializationDict));
	session->setServerSideDataHandler(this);
	
	if (options.mode == Mode::Host) {
		Logger::logDev("Starting multiplayer session as the host.");
		setState(SessionState::GameLobbyReady);
		session->host(options.maxPlayers);
		lobby = api.platform->makeMultiplayerLobby(session->getHostAddress());
	} else if (options.mode == Mode::Join && options.clientConnectTo) {
		Logger::logDev("Starting multiplayer session as a client, connecting to " + options.clientConnectTo.value());
		setState(SessionState::JoiningSession);
		session->join(options.clientConnectTo.value());
	} else if (options.mode == Mode::WaitForLobby) {
		Logger::logDev("Waiting for lobby callback...");
		setState(SessionState::WaitingForPlatformLobbyCallback);
		if (joinLobbyParameters) {
			// A callback is already pending.
			onJoinCallback();
		} else {
			joinLobbyInstance = this;
			api.platform->setJoinCallback(onPlatformJoinCallback);
		}
	}
}

SessionMultiplayer::~SessionMultiplayer()
{
	if (joinLobbyInstance == this) {
		joinLobbyInstance = nullptr;
	}

    entitySession.reset();
    session.reset();
    service.reset();
}

bool SessionMultiplayer::isMultiplayer() const
{
	return true;
}

bool SessionMultiplayer::isHost() const
{
	return host;
}

bool SessionMultiplayer::hasLocalSave() const
{
	return host;
}

bool SessionMultiplayer::hasHostAuthority() const
{
	return host;
}

Vector<Rect4f> SessionMultiplayer::getRemoteViewPorts() const
{
	if (host) {
		const auto viewPorts = entitySession->getRemoteViewPorts();
		Vector<Rect4f> result;
		result.reserve(viewPorts.size());
		for (auto& v: viewPorts) {
			result.push_back(Rect4f(v));
		}
		return result;
	}
	return {};
}

size_t SessionMultiplayer::getNumberOfPlayers() const
{
	return session->getClientCount();
}

uint8_t SessionMultiplayer::getMyClientId() const
{
	return *session->getMyPeerId();
}

SessionState SessionMultiplayer::getState() const
{
	return curState;
}

bool SessionMultiplayer::hasGameStarted() const
{
	return entitySession->isGameStarted();
}

void SessionMultiplayer::reportInitialViewPort(Rect4f viewPort)
{
	if (curState == SessionState::WaitingForInitialViewport) {
		auto& sharedData = session->getMySharedData<EntityClientSharedData>();
		sharedData.viewRect = Rect4i(viewPort);
		sharedData.markModified();
		setState(SessionState::WaitingToStart);
	}
}

void SessionMultiplayer::startOrJoinGame()
{
	if (curState == SessionState::GameLobbyReady) {
		if (host) {
			if (!hasGameStarted()) {
				setState(SessionState::PlayingGame);
				entitySession->startGame();
			}
		} else {
			if (hasGameStarted()) {
				setState(SessionState::WaitingForInitialViewport);
				entitySession->joinGame();
			}
		}
	}
}

EntityNetworkSession* SessionMultiplayer::getEntityNetworkSession()
{
	return entitySession.get();
}

const EntityNetworkSession* SessionMultiplayer::getEntityNetworkSession() const
{
	return entitySession.get();
}

NetworkSession* SessionMultiplayer::getNetworkSession()
{
	return session.get();
}

const String& SessionMultiplayer::getPlayerName() const
{
	return playerName;
}

void SessionMultiplayer::setNetworkQuality(NetworkService::Quality level)
{
	service->setSimulateQualityLevel(level);
}

MultiplayerLobby& SessionMultiplayer::getLobby()
{
	return *lobby;
}

void SessionMultiplayer::onStartSession(NetworkSession::PeerId myPeerId)
{
	setState(SessionState::JoinedSession);
}

void SessionMultiplayer::onStartGame()
{
	if (!host) {
		setState(SessionState::WaitingForInitialViewport);
	}
}

bool SessionMultiplayer::update(Time t)
{
	entitySession->update(0);
	entitySession->receiveUpdates();
	entitySession->sendUpdates();
	entitySession->update(t);

	return session->getStatus() != ConnectionStatus::Closed;
}

void SessionMultiplayer::onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId)
{
}

void SessionMultiplayer::setupInterpolators(DataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote)
{
}

bool SessionMultiplayer::isEntityInView(EntityRef entity, const EntityClientSharedData& clientData, NetworkSession::PeerId peerId)
{
	const auto* transform = entity.tryGetComponent<Transform2DComponent>();

	if (!transform) {
		return true;
	}

    // Ignore view rect if send to host
    if (peerId == 0) {
        return true;
    }

    // Rect not defined, don't send
	if (!clientData.viewRect) {
		return false;
	}

	// Send if it's in an expanded rect
	return clientData.viewRect->grow(256).contains(Vector2i(transform->getGlobalPosition()));
}

ConfigNode SessionMultiplayer::getLobbyInfo()
{
	return {};
}

bool SessionMultiplayer::setLobbyInfo(NetworkSession::PeerId fromPeerId, const ConfigNode& lobbyInfo)
{
	return false;
}

void SessionMultiplayer::onReceiveLobbyInfo(const ConfigNode& lobbyInfo)
{
}

void SessionMultiplayer::setState(SessionState state)
{
	auto oldState = curState;
	curState = state;
	Logger::logDev("Connection state: " + toString(oldState) + " -> " + toString(state));
}

bool SessionMultiplayer::setServerSideData(String uniqueKey, ConfigNode data)
{
	return false;
}

ConfigNode SessionMultiplayer::getServerSideData(String uniqueKey)
{
	return {};
}

void SessionMultiplayer::setupDictionary(SerializationDictionary& dict, std::shared_ptr<const ConfigFile> serializationDict)
{
	dict = SerializationDictionary(serializationDict->getRoot());
	//dict.setLogMissingStrings(true, 30, 100);
}

SessionMultiplayer* SessionMultiplayer::joinLobbyInstance = nullptr;
std::optional<PlatformJoinCallbackParameters> SessionMultiplayer::joinLobbyParameters;

void SessionMultiplayer::onJoinCallback()
{
	if (joinLobbyParameters && curState == SessionState::WaitingForPlatformLobbyCallback) {
		Logger::logDev("Starting multiplayer session as a client, connecting to " + joinLobbyParameters->param);
		setState(SessionState::JoiningSession);
		session->join(joinLobbyParameters->param);
		api.platform->setJoinCallback({});
		joinLobbyParameters.reset();
	} else {
		Logger::logError("Received platform join lobby callback at unexpected time.");
	}
}

void SessionMultiplayer::onPlatformJoinCallback(PlatformJoinCallbackParameters params)
{
	joinLobbyParameters = params;
	if (joinLobbyInstance != nullptr) {
		joinLobbyInstance->onJoinCallback();
	}
}
