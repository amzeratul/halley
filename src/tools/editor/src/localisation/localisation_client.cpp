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

Future<bool> LocalisationClient::postOriginalStrings(const LocOriginalDataChunk& origData) const
{
	const auto n = origData.getNumEntries();

	Json::Value root;
	auto& keys = root["keys"];
	auto& values = root["values"];

	for (int i = 0; i < n; ++i) {
		const auto& entry = origData.getEntry(i);
		keys.append(entry.key.cppStr());
		values.append(entry.value.cppStr());
	}

	Json::FastWriter writer;
	auto str = writer.write(root);

	Bytes data;
	data.resize(str.length());
	memcpy(data.data(), str.c_str(), data.size());

	const auto url = baseURL + "/strings-chunk/" + project + "/" + origData.name;
	Logger::logDev("Sending " + String::prettySize(data.size()) + " to " + url);

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
	return Future<LocOriginalData>::makeImmediate({});
}
