#pragma once
#include "localisation_data.h"

namespace Halley {
	class LocOriginalData;

	class LocalisationClient {
	public:
		LocalisationClient(WebAPI& web, String baseUrl, String project);

		Future<bool> login(const String& user, const String& password);

		Future<bool> postOriginalStrings(const LocOriginalData& origData) const;
		Future<bool> postOriginalStrings(const LocOriginalDataChunk& origData) const;
		Future<LocOriginalData> getOriginalStrings(const LocOriginalData& origData) const;

	private:
		WebAPI& web;
		String baseURL;
		String project;

		void addChunkToJsonObject(const LocOriginalDataChunk& data, Json::Value& dst) const;
		Bytes jsonToBytes(const Json::Value& src) const;
	};
}
