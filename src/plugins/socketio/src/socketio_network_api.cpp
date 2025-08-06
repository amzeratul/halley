#include "socketio_network_api.h"

#include "socketio_network_service.h"

#if defined(_WIN32) && !defined(WITH_GDK)
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace Halley;

std::unique_ptr<NetworkService> SocketIONetworkAPI::createService(NetworkProtocol protocol, int port)
{
    return std::make_unique<SocketIONetworkService>(port, protocol, IPVersion::IPv4);
}

void SocketIONetworkAPI::init()
{
#ifdef _WIN32
    WSADATA wsaData = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw Exception("Error initializing Windows Socket API", HalleyExceptions::Network);
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        throw Exception("Unexpected WSA version", HalleyExceptions::Network);
    }
#endif
}

void SocketIONetworkAPI::deInit()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

SocketIOPlatformAPI::SocketIOPlatformAPI(String playerName, const std::optional<String>& joinLobbyAddress)
    : playerName(std::move(playerName))
    , joinLobbyAddress(joinLobbyAddress)
{

}

void SocketIOPlatformAPI::init()
{

}

void SocketIOPlatformAPI::deInit()
{

}

String SocketIOPlatformAPI::getId()
{
    return "socketio";
}

void SocketIOPlatformAPI::update()
{
    if (preparingInvitation && preparingToJoinCallback) {
        preparingToJoinCallback();
        preparingInvitation = false;
        readyInvitation = true;
    }

    if (readyInvitation && joinCallback) {
        joinCallback(PlatformJoinCallbackParameters{joinLobbyAddress.value_or("127.0.0.1:6060")});
        readyInvitation = false;
    }
}

String SocketIOPlatformAPI::getPlayerName()
{
    return playerName;
}

String SocketIOPlatformAPI::getAccountId()
{
    return playerName;
}

bool SocketIOPlatformAPI::canProvideAuthToken() const
{
    return true;
}

Future<AuthTokenResult> SocketIOPlatformAPI::getAuthToken(const AuthTokenParameters& parameters)
{
    OnlineCapabilities capabilities;
    capabilities.onlinePlay = true;
    AuthTokenResult result(AuthTokenRetrievalResult::OK, capabilities);

    Promise<AuthTokenResult> promise;
    promise.setValue(std::move(result));

    return promise.getFuture();
}

void SocketIOPlatformAPI::showBrowseGamesToJoinUI()
{
    preparingInvitation = true;
}

void SocketIOPlatformAPI::setJoinCallback(PlatformJoinCallback callback)
{
    joinCallback = callback;
}

void SocketIOPlatformAPI::setPreparingToJoinCallback(PlatformPreparingToJoinCallback callback)
{
    preparingToJoinCallback = callback;
}

std::shared_ptr<NetworkService> SocketIOPlatformAPI::createNetworkService(uint16_t port)
{
    return SocketIONetworkAPI().createService(NetworkProtocol::UDP, port);
}
