#pragma once

#include "halley/core/api/halley_api_internal.h"
#include <set>

namespace Halley {
	struct SDLSaveHeaderV0
	{
		std::array<char, 8> formatId;
		uint32_t version = 1;
		uint32_t reserved = 0;
		std::array<char, 16> iv;
		uint64_t fileNameHash = 0;

		SDLSaveHeaderV0();
	};

	struct SDLSaveHeaderV1
	{
		uint64_t dataHash = 0;
	};

	struct SDLSaveHeader {
		SDLSaveHeaderV0 v0;
		SDLSaveHeaderV1 v1;

		size_t read(gsl::span<const gsl::byte> data);

		bool isValidHeader() const;
		bool isValid(const String& path, const String& key) const;
		void generateIV();

		Bytes getIV() const;
		static uint64_t computeHash(const String& path, const String& key);
	};

	class SDLSaveData : public ISaveData {
	public:
		explicit SDLSaveData(SaveDataType type, Path dir, std::optional<String> key);
		bool isReady() const override;
		Bytes getData(const String& path) override;
		void removeData(const String& path) override;
		Vector<String> enumerate(const String& root) override;
		void setData(const String& path, const Bytes& data, bool commit) override;
		void commit() override;

	private:
		SaveDataType type;
		Path dir;
		std::optional<String> key;
		std::set<String> corruptedFiles;

		String getKey() const;
		std::optional<Bytes> doGetData(const Path& path, const String& filename);
	};
}
