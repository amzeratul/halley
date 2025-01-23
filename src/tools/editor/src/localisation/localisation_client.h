#pragma once
#include "localisation_data.h"

namespace Halley {
	class LocOriginalData;

	class LocalisationClient {
	public:
		struct StringsResult {
			LocOriginalData originalLanguage;
			HashMap<String, LocTranslationData> localised;
		};

		LocalisationClient(WebAPI& web, String baseUrl, String project);

		Future<bool> login(const String& user, const String& password);

		Future<bool> postOriginalStrings(const LocOriginalData& origData) const;
		Future<bool> postOriginalStrings(const LocOriginalDataChunk& origData) const;
		Future<StringsResult> getStrings(int minVersion) const;

	private:
		WebAPI& web;
		String baseURL;
		String project;
		Vector<String> languages;

		ConfigNode getChunkConfig(const LocOriginalDataChunk& data) const;
	};
}
