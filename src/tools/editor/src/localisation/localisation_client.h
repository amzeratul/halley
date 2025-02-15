#pragma once
#include "localisation_data.h"

namespace Halley {
	class LocOriginalData;

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

		bool isConnected() const;
		const Vector<String>& getPermissions() const;
		const Vector<String>& getLanguages() const;
		bool hasPermission(std::string_view str) const;

	private:
		WebAPI& web;
		String baseURL;
		String project;

		String username;
		String password;

		String token;
		bool connected = false;
		bool connecting = false;
		Vector<String> permissions;
		Vector<String> languages;

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
		void sendPending();
		void addAuthorization(HTTPRequest& req) const;
	};
}
