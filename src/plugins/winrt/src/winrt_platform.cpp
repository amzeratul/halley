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

void WinRTPlatform::update() {}

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

void WinRTPlatform::recreateCloudSaveContainer()
{
	xbl->recreateCloudSaveContainer();
}

#endif
