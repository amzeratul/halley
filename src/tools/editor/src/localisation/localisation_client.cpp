#include "localisation_client.h"

#include "localisation_data.h"
#include "json/json.h"

using namespace Halley;

LocalisationClient::LocalisationClient(WebAPI& web, String baseURL, String project)
	: web(web)
	, baseURL(std::move(baseURL))
	, project(std::move(project))
{
	// TODO
	languages.push_back("*");
}

Future<bool> LocalisationClient::login(const String& user, const String& password)
{
	// TODO
	return Future<bool>::makeImmediate(false);
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
