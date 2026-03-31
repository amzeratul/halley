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

LocalisationClient::LocalisationClient(WebAPI& web, String origBaseURL, String project, I18NLanguage origLanguage)
	: web(web)
	, baseURL(std::move(origBaseURL))
	, project(std::move(project))
	, origLanguage(origLanguage)
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

Future<bool> LocalisationClient::putOriginalStrings(const LocStringUploadData& data, bool drySend)
{
	if (data.getChunks().empty()) {
		Logger::logInfo("No changes detected.");
		return Future<bool>::makeImmediate(true);
	}

	const auto url = "/strings-chunk/" + Encode::encodeURL(project);
	Logger::logInfo("Sending " + toString(data.getChunks().size()) + " chunks to update...");

	if (drySend) {
		Logger::logDev("Dry sending:\n" + YAMLConvert::generateYAML(data.toConfigNode()));
		return Future<bool>::makeImmediate(false);
	} else {
		return sendWithAuthorizationSimple(HTTPMethod::PUT, url, data.toConfigNode());
	}
}

Future<bool> LocalisationClient::putStringProperties(const Vector<LocStringProperties>& data)
{
	const auto url = "/strings-properties/" + Encode::encodeURL(project);

	ConfigNode entries = ConfigNode::SequenceType();
	for (const auto& entry: data) {
		entries.push_back(entry.toConfigNode());
	}

	ConfigNode payload;
	payload["entries"] = std::move(entries);

	return sendWithAuthorizationSimple(HTTPMethod::PUT, url, std::move(payload));
}

Future<std::optional<LocStringSet>> LocalisationClient::getStrings(std::optional<String> chunkId, int minVersion)
{
	auto url = "/strings/" + Encode::encodeURL(project);
	if (chunkId) {
		url += "/" + *chunkId;
	}
	url += "?minVersion=" + toString(minVersion)
		+ "&languages=" + Encode::encodeURL(String::concatList(languages, ","));

	return sendWithAuthorization(HTTPMethod::GET, url).then([origLanguage = origLanguage] (std::unique_ptr<HTTPResponse> response) -> std::optional<LocStringSet>
	{
		if (response->getResponseCode() == 200) {
			return toLocStringSet(origLanguage, JSONConvert::parseConfig(response->getBody()));
		}
		return std::nullopt;
	});
}

Future<int> LocalisationClient::getStringsVersion()
{
	const auto url = "/strings-version/" + Encode::encodeURL(project);

	return sendWithAuthorization(HTTPMethod::GET, url).then([] (std::unique_ptr<HTTPResponse> response) -> int
	{
		if (response->getResponseCode() == 200) {
			auto body = JSONConvert::parseConfig(response->getBody());
			return body["version"].asInt(-1);
		} else {
			return -2;
		}
	});
}

Future<bool> LocalisationClient::putTranslatedStrings(const LocTranslationData& translationData)
{
	const auto url = "/translated-strings/" + Encode::encodeURL(project) + "/" + Encode::encodeURL(translationData.language.getISOCode());

	return sendWithAuthorizationSimple(HTTPMethod::PUT, url, getTranslationConfig(translationData));
}

Future<Vector<LocUserData>> LocalisationClient::getUsers()
{
	const auto url = "/users";

	return sendWithAuthorization(HTTPMethod::GET, url).then([] (std::unique_ptr<HTTPResponse> response) -> Vector<LocUserData>
	{
		return JSONConvert::parseConfig(response->getBody()).asVector<LocUserData>({});
	});
}

Future<bool> LocalisationClient::createUser(const String& userId, const String& password)
{
	const auto url = "/users";

	ConfigNode payload;
	payload["username"] = userId;
	payload["password"] = password;

	return sendWithAuthorizationSimple(HTTPMethod::POST, url, payload);
}

Future<bool> LocalisationClient::deleteUser(const String& userId)
{
	const auto url = "/users/" + Encode::encodeURL(userId);

	return sendWithAuthorizationSimple(HTTPMethod::DELETE, url);
}

Future<bool> LocalisationClient::setUserAdmin(const String& userId, bool admin, const String& adminPassword)
{
	const auto url = "/users/" + Encode::encodeURL(userId) + "/admin";

	ConfigNode payload;
	payload["isAdmin"] = admin;
	payload["adminPassword"] = adminPassword;

	return sendWithAuthorizationSimple(HTTPMethod::PUT, url, payload);
}

Future<bool> LocalisationClient::setUserPassword(const String& userId, const String& newPassword)
{
	const auto url = "/users/" + Encode::encodeURL(userId) + "/password";

	ConfigNode payload;
	payload["password"] = newPassword;

	return sendWithAuthorizationSimple(HTTPMethod::PUT, url, payload);
}

Future<bool> LocalisationClient::putExternalProjectProperties(const HashMap<String, Bytes>& files)
{
	const auto url = "/external-project-properties/" + Encode::encodeURL(project);

	ConfigNode::SequenceType fileData;
	for (const auto& [k, v]: files) {
		ConfigNode entry;
		entry["path"] = k;
		entry["bytes"] = Encode::encodeBase64(v.const_byte_span());
		fileData += std::move(entry);
	}

	ConfigNode payload;
	payload["files"] = std::move(fileData);

	return sendWithAuthorizationSimple(HTTPMethod::PUT, url, payload);
}

Future<bool> LocalisationClient::setUserProjectSettings(const String& userId, const Vector<String>& languages)
{
	const auto url = "/users/" + Encode::encodeURL(userId) + "/project/" + Encode::encodeURL(project);

	ConfigNode payload;
	payload["languages"] = languages;

	return sendWithAuthorizationSimple(HTTPMethod::PUT, url, payload);
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

ConfigNode LocalisationClient::getTranslationConfig(const LocTranslationData& data) const
{
	ConfigNode result;

	ConfigNode::SequenceType keys;
	ConfigNode::SequenceType values;
	ConfigNode::SequenceType originalVersions;

	for (const auto& [k, v]: data.entries) {
		keys.push_back(ConfigNode(k));
		values.push_back(ConfigNode(v.getValue()));
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
		entry.setKey(key);
		entry.setValue(entryNode["originalValue"].asString(""));
		entry.setVersion(entryNode["originalVersion"].asInt(0));
		entry.setComment(entryNode["comment"].asString(""));
		entry.setContext(entryNode["context"].asString(""));
		entry.setPriority(tryFromString<LocPriority>(entryNode["priority"].asString("")).value_or(LocPriority::Normal));
		result.originalLanguage->getChunk(chunk).entries.push_back(entry);

		if (entryNode.hasKey("translations")) {
			for (const auto& [lang, translationNode]: entryNode["translations"].asMap()) {
				LocTranslationEntry translatedEntry;
				translatedEntry.setValue(translationNode["value"].asString(""));
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

				Logger::logInfo("Connected to server.");
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

std::unique_ptr<HTTPRequest> LocalisationClient::makeRequest(HTTPMethod method, const String& path, const ConfigNode& payload) const
{
	auto request = web.makeHTTPRequest(method, baseURL + path);
	if (payload.getType() != ConfigNodeType::Undefined) {
		request->setJsonBody(payload);
	}
	return request;
}

Future<std::unique_ptr<HTTPResponse>> LocalisationClient::sendWithAuthorization(HTTPMethod method, const String& url, const ConfigNode& payload)
{
	return sendWithAuthorization(makeRequest(method, url, payload));
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

Future<bool> LocalisationClient::sendWithAuthorizationSimple(HTTPMethod method, const String& path, const ConfigNode& payload)
{
	return sendWithAuthorizationSimple(makeRequest(method, path, payload));
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
