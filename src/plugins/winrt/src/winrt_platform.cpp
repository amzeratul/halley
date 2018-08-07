#include "winrt_system.h"
#ifdef WINDOWS_STORE

#include "winrt_platform.h"
#include "winrt_http.h"
#include "xbl_manager.h"
using namespace Halley;


WinRTPlatform::WinRTPlatform(WinRTSystem* system)
	: system(system)
{
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

class XboxLiveAuthorisationToken : public AuthorisationToken {
public:
	String getType() const override
	{
		return "xboxlive";
	}

	bool isSingleUse() const override
	{
		return false;
	}

	bool isCancellable() const override
	{
		return false;
	}

	void cancel() override
	{
		
	}

	const Bytes& getData() const override
	{
		return data; // Empty. This is because the authorisation token will be automatically inserted by IXMLHttpRequest2
	}

private:
	Bytes data;
};

Future<std::unique_ptr<AuthorisationToken>> WinRTPlatform::getAuthToken()
{
	Promise<std::unique_ptr<AuthorisationToken>> promise;
	promise.setValue(std::make_unique<XboxLiveAuthorisationToken>());
	return promise.getFuture();
}

bool WinRTPlatform::canProvideCloudSave() const
{
	return true;
}

std::shared_ptr<ISaveData> WinRTPlatform::getCloudSaveContainer(const String& containerName)
{
	return xbl->getSaveContainer(containerName);
}

#endif
