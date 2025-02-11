#pragma once
#include "localisation_data.h"

namespace Halley {
	class LocOriginalData;

	class LocalisationClient {
	public:
		struct StringsResult {
			std::optional<LocOriginalData> originalLanguage;
			HashMap<String, LocTranslationData> localised;
			bool success = false;
			int highestVersion = 0;
		};

		enum class LoginResult {
			Success,
			ServerNotFound,
			InvalidLogin
		};

		LocalisationClient(WebAPI& web, String baseUrl, String project);

		Future<LoginResult> signIn(const String& username, const String& password);
		void signOut();

		Future<bool> postOriginalStrings(const LocOriginalData& origData) const;
		Future<bool> postOriginalStrings(const LocOriginalDataChunk& origData) const;
		Future<StringsResult> getStrings(I18NLanguage origLanguage, int minVersion) const;

		bool isConnected() const;
		const Vector<String>& getPermissions() const;
		const Vector<String>& getLanguages() const;
		bool hasPermission(std::string_view str) const;

	private:
		WebAPI& web;
		String baseURL;
		String project;

		Vector<String> permissions;
		Vector<String> languages;

		String username;
		String password;
		bool connected = false;

		ConfigNode getChunkConfig(const LocOriginalDataChunk& data) const;

		static StringsResult toStringsResult(I18NLanguage origLanguage, const ConfigNode& data);
	};
}
