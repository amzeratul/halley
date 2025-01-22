#pragma once
#include "localisation_data.h"

namespace Halley {
	class LocOriginalData;
}

namespace Halley {
	class LocalisationClient {
	public:
		LocalisationClient(WebAPI& web, String baseUrl, String project);

		Future<bool> login(const String& user, const String& password);

		Future<bool> postOriginalStrings(const LocOriginalDataChunk& origData) const;
		Future<LocOriginalData> getOriginalStrings(const LocOriginalData& origData) const;

	private:
		WebAPI& web;
		String baseURL;
		String project;
	};
}
