#include "localisation_remote_client_updater.h"
#include "halley/tools/project/project.h"

using namespace Halley;

LocalisationRemoteClientUpdater::LocalisationRemoteClientUpdater(Project& project)
	: project(project)
{
}

LocalisationRemoteClientUpdater::~LocalisationRemoteClientUpdater()
{
	setListeningToClient(false);
}

void LocalisationRemoteClientUpdater::setListeningToClient(bool listening)
{
	auto devConServer = project.getDevConServer();
	if (!devConServer) {
		return;
	}

	if (listening) {
		clientLanguageHandle = devConServer->registerInterest("i18n", ConfigNode(), [=](size_t connId, ConfigNode result)
		{
			onClientI18NData(connId, std::move(result));
		});
	} else {
		if (clientLanguageHandle) {
			devConServer->unregisterInterest(*clientLanguageHandle);
			clientLanguageHandle = {};
		}
	}
}

void LocalisationRemoteClientUpdater::onClientI18NData(size_t connId, ConfigNode result)
{
	if (result.hasKey("languageCode")) {
		Logger::logInfo("Client " + toString(static_cast<int>(connId)) + " reported language: " + result["languageCode"].asString(""));
	} else {
		Logger::logInfo("Client " + toString(static_cast<int>(connId)) + " disconnected.");
	}
}
