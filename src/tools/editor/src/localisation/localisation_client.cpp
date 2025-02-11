#include "localisation_client.h"

#include "localisation_data.h"
#include "json/json.h"

using namespace Halley;

LocalisationClient::LocalisationClient(WebAPI& web, String baseURL, String project)
	: web(web)
	, baseURL(std::move(baseURL))
	, project(std::move(project))
{
}

Future<LocalisationClient::LoginResult> LocalisationClient::signIn(const String& username, const String& password)
{
	this->username = username;
	this->password = password;

	if (username.isEmpty() || password.isEmpty() || username.length() > 128 || password.length() > 128) {
		return Future<LoginResult>::makeImmediate(LoginResult::InvalidLogin);
	}

	// TODO: talk to server

	connected = true;
	languages.push_back("*");
	permissions.push_back("orig");

	return Future<LoginResult>::makeImmediate(LoginResult::Success);
}

void LocalisationClient::signOut()
{
	connected = false;
	languages = {};
	permissions = {};
	username = {};
	password = {};
}

Future<bool> LocalisationClient::postOriginalStrings(const LocOriginalData& origData) const
{
	ConfigNode root;
	auto& chunks = root["chunks"];
	for (const auto& chunk: origData.getChunks()) {
		chunks.push_back(getChunkConfig(chunk));
	}

	const auto data = JSONConvert::generateJSON(root).toBytes();

	const auto url = baseURL + "/strings-chunk/" + Encode::encodeURL(project);

	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setBody("application/json", data);
	return request->send().then([] (std::unique_ptr<HTTPResponse> response)
	{
		return response->getResponseCode() == 200;
	});
}

Future<bool> LocalisationClient::postOriginalStrings(const LocOriginalDataChunk& origData) const
{
	const auto data = JSONConvert::generateJSON(getChunkConfig(origData)).toBytes();

	const auto url = baseURL + "/strings-chunk/" + Encode::encodeURL(project) + "/" + Encode::encodeURL(origData.name);
	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setBody("application/json", data);

	return request->send().then([] (std::unique_ptr<HTTPResponse> response)
	{
		Logger::logDev("Got response: " + toString(response->getResponseCode()));
		return response->getResponseCode() == 200;
	});
}

Future<LocalisationClient::StringsResult> LocalisationClient::getStrings(I18NLanguage origLanguage, int minVersion) const
{
	const auto url = baseURL + "/strings/" + Encode::encodeURL(project)
		+ "?minVersion=" + toString(minVersion)
		+ "&languages=" + Encode::encodeURL(String::concatList(languages, ","));
	auto request = web.makeHTTPRequest(HTTPMethod::GET, url);

	return request->send().then([origLanguage] (std::unique_ptr<HTTPResponse> response) -> StringsResult
	{
		if (response->getResponseCode() == 200) {
			return toStringsResult(origLanguage, JSONConvert::parseConfig(response->getBody()));
		}
		return {};
	});
}

bool LocalisationClient::isConnected() const
{
	return connected;
}

const Vector<String>& LocalisationClient::getPermissions() const
{
	return permissions;
}

const Vector<String>& LocalisationClient::getLanguages() const
{
	return languages;
}

bool LocalisationClient::hasPermission(std::string_view str) const
{
	return permissions.contains(str);
}

ConfigNode LocalisationClient::getChunkConfig(const LocOriginalDataChunk& data) const
{
	ConfigNode result;
	result["chunkId"] = data.name;

	auto& keys = result["keys"];
	auto& values = result["values"];

	const auto n = data.getNumEntries();
	for (int i = 0; i < n; ++i) {
		const auto& entry = data.getEntry(i);
		keys.push_back(ConfigNode(entry.key));
		values.push_back(ConfigNode(entry.value));
	}

	return result;
}

LocalisationClient::StringsResult LocalisationClient::toStringsResult(I18NLanguage origLanguage, const ConfigNode& data)
{
	int version = 0;

	StringsResult result;
	result.originalLanguage = LocOriginalData();

	for (const auto& entryNode: data.asSequence()) {
		const auto key = entryNode["key"].asString();
		const auto chunk = entryNode["chunk"].asString("");
		version = std::max(version, entryNode["version"].asInt(0));

		LocalisationDataEntry entry;
		entry.key = key;
		entry.value = entryNode["originalValue"].asString("");
		entry.version = entryNode["originalVersion"].asInt(0);
		entry.comment = entryNode["comment"].asString("");
		entry.context = entryNode["context"].asString("");
		result.originalLanguage->getChunk(chunk).entries.push_back(entry);

		if (entryNode.hasKey("translations")) {
			for (const auto& [lang, translationNode]: entryNode["translations"].asMap()) {
				LocTranslationEntry translatedEntry;
				translatedEntry.value = translationNode["value"].asString("");
				translatedEntry.origVersion = translationNode["version"].asInt(0);

				result.localised[lang].entries[key] = translatedEntry;
			}
		}
	}

	for (auto& [lang, loc] : result.localised) {
		loc.language = I18NLanguage(lang);
	}

	result.originalLanguage->setLanguage(std::move(origLanguage));
	result.originalLanguage->indexData();

	result.success = true;
	result.highestVersion = version;

	return result;
}
