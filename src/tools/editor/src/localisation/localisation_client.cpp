#include "localisation_client.h"

#include "localisation_data.h"
#include "json/json.h"

using namespace Halley;

LocProjectData::LocProjectData(const ConfigNode& node)
{
	languages = node["languages"].asVector<String>({});
	std::sort(languages.begin(), languages.end());
}

bool LocProjectData::operator==(const LocProjectData& other) const
{
	return languages == other.languages;
}

bool LocProjectData::operator!=(const LocProjectData& other) const
{
	return !(*this == other);
}

LocUserData::LocUserData(const ConfigNode& node)
{
	username = node["username"].asString("");
	isAdmin = node["isAdmin"].asBool(false);
	projects = node["projects"].asHashMap<String, LocProjectData>();
}

const LocProjectData& LocUserData::getProject(const String& projectId) const
{
	const auto projIter = projects.find(projectId);
	if (projIter != projects.end()) {
		return projIter->second;
	} else {
		static LocProjectData dummy;
		return dummy;
	}
}

LocProjectData& LocUserData::getProject(const String& projectId)
{
	return projects[projectId];
}

bool LocUserData::operator==(const LocUserData& other) const
{
	return username == other.username
		&& isAdmin == other.isAdmin
		&& projects == other.projects;
}

bool LocUserData::operator!=(const LocUserData& other) const
{
	return !(*this == other);
}

LocalisationClient::LocalisationClient(WebAPI& web, String origBaseURL, String project)
	: web(web)
	, baseURL(std::move(origBaseURL))
	, project(std::move(project))
{
	if (baseURL.endsWith("/")) {
		baseURL = baseURL.left(baseURL.size() - 1);
	}
}

Future<LocalisationClient::LoginResult> LocalisationClient::signIn(const String& username, const String& password)
{
	Logger::logInfo("Connecting to " + baseURL + "...");

	if (connecting) {
		Logger::logError("Already trying to connect to server");
		return Future<LoginResult>::makeImmediate(LoginResult::InvalidLogin);
	}

	this->username = username;
	this->password = password;

	if (username.isEmpty() || password.isEmpty() || username.length() > 128 || password.length() > 128) {
		return Future<LoginResult>::makeImmediate(LoginResult::InvalidLogin);
	}

	ConfigNode reqInfo;
	reqInfo["username"] = username;
	reqInfo["password"] = password;
	reqInfo["project"] = project;
	const auto reqBody = JSONConvert::generateJSON(reqInfo).toBytes();

	const auto url = baseURL + "/sessions";
	auto request = web.makeHTTPRequest(HTTPMethod::POST, url);
	request->setBody("application/json", reqBody);

	connecting = true;
	return request->send().then(Executors::getMainUpdateThread(), [=] (std::unique_ptr<HTTPResponse> response) -> LoginResult
	{
		connecting = false;
		if (response->getResponseCode() == 0) {
			setToken("");
			return LoginResult::ServerNotFound;
		} else if (response->getResponseCode() == 200) {
			const auto responseBody = JSONConvert::parseConfig(response->getBody());
			setToken(responseBody["token"].asString(""));
			return connected ? LoginResult::Success : LoginResult::InvalidLogin;
		} else {
			const auto responseBody = JSONConvert::parseConfig(response->getBody());
			Logger::logError("Error attempting to login: " + responseBody["errorMsg"].asString(""));
			setToken("");
			return LoginResult::InvalidLogin;
		}
	});
}

void LocalisationClient::signOut()
{
	connected = false;
	languages = {};
	permissions = {};
	username = {};
	password = {};
}

Future<bool> LocalisationClient::putOriginalStrings(const LocOriginalData& origData)
{
	ConfigNode payload;
	auto& chunks = payload["chunks"];
	for (const auto& chunk: origData.getChunks()) {
		chunks.push_back(getChunkConfig(chunk));
	}

	const auto url = baseURL + "/strings-chunk/" + Encode::encodeURL(project);

	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setJsonBody(payload);
	return sendWithAuthorizationSimple(std::move(request));
}

Future<bool> LocalisationClient::putOriginalStrings(const LocOriginalDataChunk& origData)
{
	const auto url = baseURL + "/strings-chunk/" + Encode::encodeURL(project) + "/" + Encode::encodeURL(origData.name);
	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setJsonBody(getChunkConfig(origData));

	return sendWithAuthorizationSimple(std::move(request));
}

Future<std::optional<LocStringSet>> LocalisationClient::getStrings(I18NLanguage origLanguage, int minVersion)
{
	const auto url = baseURL + "/strings/" + Encode::encodeURL(project)
		+ "?minVersion=" + toString(minVersion)
		+ "&languages=" + Encode::encodeURL(String::concatList(languages, ","));
	auto request = web.makeHTTPRequest(HTTPMethod::GET, url);

	return sendWithAuthorization(std::move(request)).then([origLanguage] (std::unique_ptr<HTTPResponse> response) -> std::optional<LocStringSet>
	{
		if (response->getResponseCode() == 200) {
			return toLocStringSet(origLanguage, JSONConvert::parseConfig(response->getBody()));
		}
		return std::nullopt;
	});
}

Future<bool> LocalisationClient::putTranslatedStrings(I18NLanguage language, const LocTranslationData& translationData)
{
	const auto url = baseURL + "/translated-strings/" + Encode::encodeURL(project) + "/" + Encode::encodeURL(language.getISOCode());
	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setJsonBody(getTranslationConfig(translationData));

	return sendWithAuthorizationSimple(std::move(request));
}

Future<Vector<LocUserData>> LocalisationClient::getUsers()
{
	const auto url = baseURL + "/users";
	auto request = web.makeHTTPRequest(HTTPMethod::GET, url);

	return sendWithAuthorization(std::move(request)).then([] (std::unique_ptr<HTTPResponse> response) -> Vector<LocUserData>
	{
		return JSONConvert::parseConfig(response->getBody()).asVector<LocUserData>({});
	});
}

Future<bool> LocalisationClient::createUser(const String& userId, const String& password)
{
	ConfigNode payload;
	payload["username"] = userId;
	payload["password"] = password;

	const auto url = baseURL + "/users";
	auto request = web.makeHTTPRequest(HTTPMethod::POST, url);
	request->setJsonBody(payload);

	return sendWithAuthorizationSimple(std::move(request));
}

Future<bool> LocalisationClient::deleteUser(const String& userId)
{
	const auto url = baseURL + "/users/" + Encode::encodeURL(userId);
	auto request = web.makeHTTPRequest(HTTPMethod::DELETE, url);

	return sendWithAuthorizationSimple(std::move(request));
}

Future<bool> LocalisationClient::setUserAdmin(const String& userId, bool admin, const String& adminPassword)
{
	ConfigNode payload;
	payload["isAdmin"] = admin;
	payload["adminPassword"] = adminPassword;

	const auto url = baseURL + "/users/" + Encode::encodeURL(userId) + "/admin";
	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setJsonBody(payload);

	return sendWithAuthorizationSimple(std::move(request));
}

Future<bool> LocalisationClient::setUserPassword(const String& userId, const String& newPassword)
{
	ConfigNode payload;
	payload["password"] = newPassword;

	const auto url = baseURL + "/users/" + Encode::encodeURL(userId) + "/password";
	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setJsonBody(payload);

	return sendWithAuthorizationSimple(std::move(request));
}

Future<bool> LocalisationClient::setUserProjectSettings(const String& userId, const Vector<String>& languages)
{
	ConfigNode payload;
	payload["languages"] = languages;

	const auto url = baseURL + "/users/" + Encode::encodeURL(userId) + "/project/" + Encode::encodeURL(project);
	auto request = web.makeHTTPRequest(HTTPMethod::PUT, url);
	request->setJsonBody(payload);

	return sendWithAuthorizationSimple(std::move(request));
}

const String& LocalisationClient::getMyUsername() const
{
	return username;
}

bool LocalisationClient::isConnected() const
{
	return connected;
}

const Vector<String>& LocalisationClient::getLanguages() const
{
	return languages;
}

bool LocalisationClient::hasPermission(std::string_view str) const
{
	return permissions.contains(str);
}

bool LocalisationClient::isAdmin() const
{
	return hasPermission("admin");
}

const String& LocalisationClient::getProject() const
{
	return project;
}

ConfigNode LocalisationClient::getChunkConfig(const LocOriginalDataChunk& data) const
{
	ConfigNode keys;
	ConfigNode values;

	const auto n = data.getNumEntries();
	for (int i = 0; i < n; ++i) {
		const auto& entry = data.getEntry(i);
		keys.push_back(ConfigNode(entry.key));
		values.push_back(ConfigNode(entry.value));
	}

	ConfigNode result;
	result["chunkId"] = data.name;
	result["keys"] = std::move(keys);
	result["values"] = std::move(values);
	return result;
}

ConfigNode LocalisationClient::getTranslationConfig(const LocTranslationData& data) const
{
	ConfigNode result;

	ConfigNode::SequenceType keys;
	ConfigNode::SequenceType values;
	ConfigNode::SequenceType originalVersions;

	for (const auto& [k, v]: data.entries) {
		keys.push_back(ConfigNode(k));
		values.push_back(ConfigNode(v.value));
		originalVersions.push_back(ConfigNode(v.origVersion));
	}

	result["keys"] = std::move(keys);
	result["values"] = std::move(values);
	result["originalVersions"] = std::move(originalVersions);

	return result;
}

LocStringSet LocalisationClient::toLocStringSet(I18NLanguage origLanguage, const ConfigNode& data)
{
	int version = 0;

	LocStringSet result;
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
				translatedEntry.version = translationNode["version"].asInt(0);
				translatedEntry.origVersion = translationNode["origVersion"].asInt(0);

				result.localised[lang].entries[key] = translatedEntry;
			}
		}
	}

	for (auto& [lang, loc] : result.localised) {
		loc.language = I18NLanguage(lang);
	}

	result.originalLanguage->setLanguage(std::move(origLanguage));
	result.originalLanguage->indexData();

	result.highestVersion = version;

	return result;
}

void LocalisationClient::setToken(String _token)
{
	token = std::move(_token);

	connected = false;
	languages.clear();
	permissions.clear();
	tokenExpiration = {};

	try {
		if (!token.isEmpty()) {
			const auto split = token.split('.');
			if (split.size() == 3) {
				const auto jsonClaims = String(Encode::decodeBase64(split[1]));
				Logger::logDev("Auth token claims:\n" + jsonClaims);
				const auto claims = JSONConvert::parseConfig(jsonClaims);

				tokenExpiration = claims["exp"].asOptional<int64_t>();
				permissions = claims["permissions"].asVector<String>({});
				languages = claims["languages"].asVector<String>({});

				connected = true;
			}
		}
	} catch (...) {
		Logger::logError("Invalid token returned from server: " + token);
	}

	if (connected) {
		sendPending();
	}
}

Future<std::unique_ptr<HTTPResponse>> LocalisationClient::sendWithAuthorization(std::unique_ptr<HTTPRequest> request)
{
	if (tokenExpiration) {
		// Check for expiration
		const auto secondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		int64_t secondsLeft = *tokenExpiration - secondsSinceEpoch;
		if (secondsLeft < 60) {
			// Expire token
			Logger::logInfo("Localisation token expiring, will obtain a new one");
			setToken("");
		}
	}

	if (connected) {
		addAuthorization(*request);
		return request->send();
	} else {
		auto& pending = pendingRequests.emplace_back();
		pending.request = std::move(request);

		if (!connecting) {
			signIn(username, password);
		}

		return pending.promise.getFuture();
	}
}

Future<bool> LocalisationClient::sendWithAuthorizationSimple(std::unique_ptr<HTTPRequest> request)
{
	return sendWithAuthorization(std::move(request)).then([=] (std::unique_ptr<HTTPResponse> response) {
		if (response->getResponseCode() == 200) {
			return true;
		} else {
			Logger::logError("Request returned " + toString(response->getResponseCode()));
			return false;
		}
	});
}

void LocalisationClient::sendPending()
{
	if (connected) {
		for (auto& pending: pendingRequests) {
			addAuthorization(*pending.request);
			pending.request->send().then([=, promise = std::move(pending.promise)](std::unique_ptr<HTTPResponse> response) mutable
			{
				promise.setValue(std::move(response));
			});
		}
		pendingRequests.clear();
	}
}

void LocalisationClient::addAuthorization(HTTPRequest& req) const
{
	req.setHeader("Authorization", "Bearer " + token);
}
