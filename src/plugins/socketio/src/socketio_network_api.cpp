#include "socketio_network_api.h"

#include "socketio_network_service.h"

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

void SocketIOPlatformAPI::init()
{

}

void SocketIOPlatformAPI::deInit()
{

}

SocketIOPlatformAPI::SocketIOPlatformAPI(String playerName)
    : playerName(std::move(playerName))
{

}

String SocketIOPlatformAPI::getId()
{
    return "socketio";
}

void SocketIOPlatformAPI::update()
{

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

std::shared_ptr<NetworkService> SocketIOPlatformAPI::createNetworkService(uint16_t port)
{
    return SocketIONetworkAPI().createService(NetworkProtocol::UDP, port);
}
