#include "dummy_platform.h"

using namespace Halley;

void DummyPlatformAPI::init() {}
void DummyPlatformAPI::deInit() {}

String DummyPlatformAPI::getId()
{
	return "dummy";
}

void DummyPlatformAPI::update() {}

bool DummyPlatformAPI::canProvideAuthToken() const
{
	return false;
}

Future<AuthTokenResult> DummyPlatformAPI::getAuthToken(const AuthTokenParameters& parameters)
{
	return {};
}
