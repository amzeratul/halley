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

Future<LocalisationClient::StringsResult> LocalisationClient::getStrings(int minVersion) const
{
	const auto url = baseURL + "/strings/" + Encode::encodeURL(project)
		+ "?minVersion=" + toString(minVersion)
		+ "&languages=" + Encode::encodeURL(String::concatList(languages, ","));
	auto request = web.makeHTTPRequest(HTTPMethod::GET, url);

	return request->send().then([] (std::unique_ptr<HTTPResponse> response) -> StringsResult
	{
		if (response->getResponseCode() == 200) {
			auto result = JSONConvert::parseConfig(response->getBody());
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
