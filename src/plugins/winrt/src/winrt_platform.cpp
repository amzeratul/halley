#include "winrt_system.h"
#ifdef WINDOWS_STORE

#include "winrt_platform.h"
#include "winrt_http.h"
#include "xbl_manager.h"

using namespace Halley;


WinRTPlatform::WinRTPlatform(WinRTSystem* system)
	: system(system)
{
	system->setPlatform(this);
}

void WinRTPlatform::init()
{
	xbl = std::make_shared<XBLManager>();
	xbl->init();
}

void WinRTPlatform::deInit()
{
	xbl.reset();
}

void WinRTPlatform::update()
{
	xbl->update();

	if (xbl->incommingInvitation()) {
		xbl->acceptInvitation();
	}
}

std::unique_ptr<HTTPRequest> WinRTPlatform::makeHTTPRequest(const String& method, const String& url)
{
	return std::make_unique<WinRTHTTPRequest>(method, url);
}

bool WinRTPlatform::canProvideAuthToken() const
{
	return true;
}

Future<AuthTokenResult> WinRTPlatform::getAuthToken(const AuthTokenParameters& parameters)
{
	return xbl->getAuthToken(parameters);
}

bool WinRTPlatform::canProvideCloudSave() const
{
	return true;
}

std::shared_ptr<ISaveData> WinRTPlatform::getCloudSaveContainer(const String& containerName)
{
	return xbl->getSaveContainer(containerName);
}

void WinRTPlatform::setAchievementProgress(const String& achievementId, int currentProgress, int maximumValue)
{
	xbl->setAchievementProgress(achievementId, currentProgress, maximumValue);
}

bool WinRTPlatform::isAchievementUnlocked(const String& achievementId, bool defaultValue)
{
	return xbl->isAchievementUnlocked(achievementId, defaultValue);
}

std::unique_ptr<MultiplayerSession> WinRTPlatform::makeMultiplayerSession(const String& key)
{
	return xbl->makeMultiplayerSession(key);
}

bool WinRTPlatform::multiplayerProcessingInvitation()
{
	return (xbl->getMultiplayerStatus()==MultiplayerStatus::Initializing);
}

bool WinRTPlatform::multiplayerProcessingInvitationError()
{
	return (xbl->getMultiplayerStatus()==MultiplayerStatus::Error);
}

void WinRTPlatform::multiplayerInvitationCancel()
{
	return xbl->closeMultiplayer();	
}

void WinRTPlatform::recreateCloudSaveContainer()
{
	xbl->recreateCloudSaveContainer();
}

String WinRTPlatform::getPlayerName()
{
	return xbl->getPlayerName();
}

bool WinRTPlatform::playerHasLoggedOut()
{
	return xbl->playerHasLoggedOut();
}

void WinRTPlatform::setJoinCallback(PlatformJoinCallback callback)
{
	xbl->setJoinCallback(callback);
}

void WinRTPlatform::setPreparingToJoinCallback(PlatformPreparingToJoinCallback callback)
{
	xbl->setPreparingToJoinCallback(callback);
}

void WinRTPlatform::showPlayerInfo(String playerId)
{
	xbl->showPlayerInfo(playerId);
}

void WinRTPlatform::invitationArrived(const std::wstring& uri)
{ 
	xbl->invitationArrived(uri);
}

void WinRTPlatform::setProfanityCheckForbiddenWordsList(std::vector<String> words)
{
	xbl->setProfanityCheckForbiddenWordsList(std::move(words));
}

Future<String> WinRTPlatform::performProfanityCheck(String text)
{
	Promise<String> promise;
	promise.setValue(xbl->performProfanityCheck(text));
	return promise.getFuture();
}

#endif
