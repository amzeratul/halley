#pragma once
#include "localisation_data.h"

namespace Halley {
	class LocOriginalData;

	class LocProjectData {
	public:
		Vector<String> languages;

		LocProjectData() = default;
		LocProjectData(const ConfigNode& node);

		bool operator==(const LocProjectData& other) const;
		bool operator!=(const LocProjectData& other) const;
	};

	class LocUserData {
	public:
		String username;
		bool isAdmin;
		HashMap<String, LocProjectData> projects;

		LocUserData() = default;
		LocUserData(const ConfigNode& node);

		const LocProjectData& getProject(const String& projectId) const;
		LocProjectData& getProject(const String& projectId);

		bool operator==(const LocUserData& other) const;
		bool operator!=(const LocUserData& other) const;
	};

	class LocalisationClient {
	public:
		enum class LoginResult {
			Success,
			ServerNotFound,
			InvalidLogin
		};

		LocalisationClient(WebAPI& web, String baseUrl, String project);

		Future<LoginResult> signIn(const String& username, const String& password);
		void signOut();

		Future<bool> putOriginalStrings(const LocOriginalData& origData);
		Future<bool> putOriginalStrings(const LocOriginalDataChunk& origData);
		Future<std::optional<LocStringSet>> getStrings(I18NLanguage origLanguage, int minVersion);

		Future<bool> putTranslatedStrings(I18NLanguage language, const LocTranslationData& translationData);

		Future<Vector<LocUserData>> getUsers();
		Future<bool> createUser(const String& userId, const String& password);
		Future<bool> deleteUser(const String& userId);
		Future<bool> setUserAdmin(const String& userId, bool admin, const String& adminPassword);
		Future<bool> setUserProjectSettings(const String& userId, const Vector<String>& languages);
		Future<bool> setUserPassword(const String& userId, const String& newPassword);

		const String& getMyUsername() const;
		bool isConnected() const;

		const Vector<String>& getLanguages() const;
		bool hasPermission(std::string_view str) const;
		bool isAdmin() const;
		const String& getProject() const;

	private:
		WebAPI& web;
		String baseURL;
		String project;

		String username;
		String password;

		String token;
		std::optional<long long> tokenExpiration;
		Vector<String> permissions;
		Vector<String> languages;

		bool connected = false;
		bool connecting = false;

		struct PendingRequest {
			Promise<std::unique_ptr<HTTPResponse>> promise;
			std::unique_ptr<HTTPRequest> request;
		};
		std::list<PendingRequest> pendingRequests;

		ConfigNode getChunkConfig(const LocOriginalDataChunk& data) const;
		ConfigNode getTranslationConfig(const LocTranslationData& data) const;

		static LocStringSet toLocStringSet(I18NLanguage origLanguage, const ConfigNode& data);

		void setToken(String token);
		Future<std::unique_ptr<HTTPResponse>> sendWithAuthorization(std::unique_ptr<HTTPRequest> request);
		Future<bool> sendWithAuthorizationSimple(std::unique_ptr<HTTPRequest> request);
		void sendPending();
		void addAuthorization(HTTPRequest& req) const;
	};
}
