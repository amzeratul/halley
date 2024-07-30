#if defined(WITH_GDK)

#include "os_gdk.h"

#include <XGameRuntime.h>
#include <XGameSaveFiles.h>

using namespace Halley;

OSGDK::OSGDK()
{
    /*
     * NOTE: XGameRuntime should be initialized already by SDL_RunApp().
     *
     * This does a minimal user signOn, adapted from SDL_GDKGetDefaultUser().
     * We do this here because we need the XUID to set up a user save path.
     */
    XAsyncBlock async = {};

#ifdef _GAMING_XBOX
    XUserAddOptions options = XUserAddOptions::AddDefaultUserAllowingUI;
#else
    XUserAddOptions options = XUserAddOptions::AddDefaultUserSilently;
#endif

    HRESULT hr = XUserAddAsync(options, &async);

    if (SUCCEEDED(hr)) {
	    do {
		    hr = XUserAddResult(&async, &userHandle);
	    } while (hr == E_PENDING);

        if (SUCCEEDED(hr)) {
	        hr = XUserGetId(userHandle, &userId);
        }
    }

    if (SUCCEEDED(hr) && userId != 0) {
        computerData.userName = toString(userId);
    } else {
	    computerData.userName = "defaultUser";
    }

}

OSGDK::~OSGDK()
{
    if (userHandle != nullptr) {
		XUserCloseHandle(userHandle);
    }
}

ComputerData OSGDK::getComputerData()
{
	return computerData;
}

String OSGDK::getUserDataDir()
{
    /*
     * On GDK desktop this returns %LOCALAPPDATA%\{Identity Name}. On console, it's
     * something like "R:\\".
     *
     * On console this *requires* <PersistentLocalStorage> in MicrosoftGame.config.
     */
    size_t pathSize = 0;
    HRESULT hr = XPersistentLocalStorageGetPathSize(&pathSize);

    if (FAILED(hr)) {
		throw Exception("getUserDataDir", HalleyExceptions::SystemPlugin);
    }

    String result;
    result.setSize(pathSize);

    size_t pathUsed = 0;
    hr = XPersistentLocalStorageGetPath(pathSize, &result[0], &pathUsed);

    if (FAILED(hr)) {
		throw Exception("getUserDataDir", HalleyExceptions::SystemPlugin);
    }

    // The path is null-terminated.
    result.setSize(pathUsed - 1);

    /*
     * We append the XUID to manage per-user settings.
     */
    if (userId != 0) {
	    if (result[pathUsed - 2] != '\\') {
		    result += "\\";
	    }
	    result = result + toString(userId) + "\\";
    }

    return result;
}

void OSGDK::openURL(const String& url)
{
	if (url.startsWith("http://") || url.startsWith("https://")) {
        XLaunchUri(userHandle, url.c_str());
	}
}

#endif
