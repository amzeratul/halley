#pragma once

#include "halley/core/api/halley_api_internal.h"

namespace Halley {
	struct SDLSaveHeader {
		std::array<char, 8> formatId;
		uint32_t version = 0;
		uint32_t reserved = 0;
		std::array<char, 16> iv;
		uint64_t fileNameHash = 0;

		bool isValid() const;
		void init();
		void generateIV();

		Bytes getIV() const;
		static uint64_t computeHash(const String& path, const String& key);
	};

	class SDLSaveData : public ISaveData {
	public:
		explicit SDLSaveData(SaveDataType type, Path dir, Maybe<String> key);
		bool isReady() const override;
		Bytes getData(const String& path) override;
		void removeData(const String& path) override;
		std::vector<String> enumerate(const String& root) override;
		void setData(const String& path, const Bytes& data, bool commit) override;
		void commit() override;

	private:
		SaveDataType type;
		Path dir;
		Maybe<String> key;

		String getKey() const;
	};
}
