#pragma once

#include "halley/api/halley_api_internal.h"
#include <set>

namespace Halley {
	struct SDLSaveHeaderV0
	{
		std::array<char, 8> formatId;
		uint32_t version = 2;
		uint32_t reserved = 0;
		std::array<uint8_t, 16> iv;
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
		bool isValid(const String& path, const Vector<uint8_t>& key) const;
		void generateIV();

		Vector<uint8_t> getIV() const;
		static uint64_t computeHash(const String& path, const Vector<uint8_t>& key);
	};

	class SDLSaveData : public ISaveData {
	public:
		explicit SDLSaveData(SaveDataType type, Path dir, std::optional<String> key);
		bool isReady() const override;
		Bytes getData(const String& path) override;
		void removeData(const String& path) override;
		Vector<String> enumerate(const String& root) override;
		void setData(const String& path, const Bytes& data, bool commit, bool log) override;
		void commit() override;

	private:
		SaveDataType type;
		Path dir;
		std::optional<String> key;
		std::set<String> corruptedFiles;

		Vector<uint8_t> getKeyV2() const;
		Vector<uint8_t> getKeyV1() const;
		std::optional<Bytes> doGetData(const Path& path, const String& filename);
	};
}
