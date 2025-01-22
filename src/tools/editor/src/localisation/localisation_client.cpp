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

Future<bool> LocalisationClient::login(const String& user, const String& password)
{
	// TODO
	return Future<bool>::makeImmediate(false);
}

Future<bool> LocalisationClient::postOriginalStrings(const LocOriginalData& origData) const
{
	Json::Value root;
	auto& chunks = root["chunks"];

	for (const auto& chunk: origData.getChunks()) {
		auto& chunkRoot = chunks.append({});
		addChunkToJsonObject(chunk, chunkRoot);
	}

	const auto data = jsonToBytes(root);

	const auto url = baseURL + "/strings-chunk/" + Encode::encodeURL(project);
	Logger::logDev("Sending " + String::prettySize(data.size()) + " to " + url);

	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setBody("application/json", data);
	return request->send().then([] (std::unique_ptr<HTTPResponse> response)
	{
		Logger::logDev("Got response: " + toString(response->getResponseCode()));
		return response->getResponseCode() == 200;
	});
}

Future<bool> LocalisationClient::postOriginalStrings(const LocOriginalDataChunk& origData) const
{
	Json::Value root;
	addChunkToJsonObject(origData, root);
	const auto data = jsonToBytes(root);

	const auto url = baseURL + "/strings-chunk/" + Encode::encodeURL(project) + "/" + Encode::encodeURL(origData.name);
	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setBody("application/json", data);

	return request->send().then([] (std::unique_ptr<HTTPResponse> response)
	{
		Logger::logDev("Got response: " + toString(response->getResponseCode()));
		return response->getResponseCode() == 200;
	});
}

Future<LocOriginalData> LocalisationClient::getOriginalStrings(const LocOriginalData& origData) const
{
	int minVersion = 0; // TODO
	String languages = ""; // TODO

	const auto url = baseURL + "/strings/" + Encode::encodeURL(project) + "?minVersion=" + toString(minVersion) + "&languages=" + Encode::encodeURL(languages);
	auto request = web.makeHTTPRequest(HTTPMethod::GET, url);

	return request->send().then([] (std::unique_ptr<HTTPResponse> response) -> LocOriginalData
	{
		if (response->getResponseCode() == 200) {
			auto body = response->getBody();
			Logger::logDev("Got a response of size " + String::prettySize(body.size()));
		}
		return {};
	});
}

void LocalisationClient::addChunkToJsonObject(const LocOriginalDataChunk& data, Json::Value& dst) const
{
	dst["chunkId"] = data.name.cppStr();

	auto& keys = dst["keys"];
	auto& values = dst["values"];

	const auto n = data.getNumEntries();
	for (int i = 0; i < n; ++i) {
		const auto& entry = data.getEntry(i);
		keys.append(entry.key.cppStr());
		values.append(entry.value.cppStr());
	}
}

Bytes LocalisationClient::jsonToBytes(const Json::Value& src) const
{
	Json::FastWriter writer;
	auto str = writer.write(src);

	Bytes data;
	data.resize(str.length());
	memcpy(data.data(), str.c_str(), data.size());

	return data;
}
