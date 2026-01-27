#include "localisation_remote_client_updater.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"

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
		if (!clientLanguageHandle) {
			clientLanguageHandle = devConServer->registerInterest("i18n", ConfigNode(), [=](size_t connId, ConfigNode result)
			{
				onClientI18NData(connId, std::move(result));
			});
		}
	} else {
		if (clientLanguageHandle) {
			devConServer->unregisterInterest(*clientLanguageHandle);
			clientLanguageHandle = {};
		}
	}
}

void LocalisationRemoteClientUpdater::setLocalStrings(const LocStringSet* local)
{
	localSet = local;
}

void LocalisationRemoteClientUpdater::setRemoteStrings(const LocStringSet* remote)
{
	remoteSet = remote;
}

void LocalisationRemoteClientUpdater::onStringsUpdated()
{
	for (const auto& [id, c]: clients) {
		sendStringsToClient(id, std::nullopt);
	}
}

void LocalisationRemoteClientUpdater::onStringUpdated(const String& key)
{
	for (const auto& [id, c]: clients) {
		sendStringsToClient(id, key);
	}
}

void LocalisationRemoteClientUpdater::onClientI18NData(size_t connId, ConfigNode result)
{
	if (result.hasKey("languageCode")) {
		clients[connId].language = I18NLanguage(result["languageCode"].asString(""));
		clients[connId].strings = {};
		sendStringsToClient(connId, {});
	} else {
		clients.erase(connId);
	}
}

void LocalisationRemoteClientUpdater::sendStringsToClient(size_t connId, std::optional<String> key)
{
	auto* conn = project.getDevConServer()->tryGetConnection(connId);
	if (!conn) {
		clients.erase(connId);
		return;
	}

	auto& client = clients[connId];
	conn->sendI18NStrings(client.language, makeStringDelta(client, key));
}

HashMap<String, String> LocalisationRemoteClientUpdater::makeStringDelta(Client& client, const std::optional<String>& key)
{
	if (client.language == project.getProperties().getOriginalLanguage()) {
		return makeOriginalStringDelta(client, key);
	} else {
		return makeTranslatedStringDelta(client, key);
	}
}

HashMap<String, String> LocalisationRemoteClientUpdater::makeOriginalStringDelta(Client& client, const std::optional<String>& filterKey)
{
	HashMap<String, String> result;

	// TODO: not supported for original language atm

	return result;
}

HashMap<String, String> LocalisationRemoteClientUpdater::makeTranslatedStringDelta(Client& client, const std::optional<String>& filterKey)
{
	const auto code = client.language.getISOCode();
	const LocTranslationData* src = localSet && localSet->localised.contains(code) ?
		&localSet->localised.at(code) :
		(remoteSet && remoteSet->localised.contains(code) ? &remoteSet->localised.at(code) : nullptr);
	if (!src) {
		return {};
	}

	HashMap<String, String> result;

	if (filterKey) {
		if (auto iter = src->entries.find(*filterKey); iter != src->entries.end()) {
			result[*filterKey] = iter->second.getValue();
			client.strings[*filterKey] = iter->second.getValue();
		}
	} else {
		for (const auto& [key, entry]: src->entries) {
			if (const auto iter = client.strings.find(key); iter != client.strings.end()) {
				if (iter->second != entry.getValue()) {
					client.strings[key] = entry.getValue();
					result[key] = entry.getValue();
				}
			} else {
				client.strings[key] = entry.getValue();
				result[key] = entry.getValue();
			}
		}
	}

	return result;
}
