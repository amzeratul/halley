#pragma once
#include "halley/utils/utils.h"
#include "halley/text/halleystring.h"
#include <memory>

namespace Halley {
	class Deserializer;
	class Serializer;
	class AssetDatabase;
	class ResourceDataReader;

	struct AssetPackHeader {
		std::array<char, 8> identifier;
		uint64_t assetDbSize;
		uint64_t dataStartPos;
	};

    class AssetPack {
    public:
		AssetPack();
		AssetPack(AssetPack&& other);
		AssetPack(std::unique_ptr<ResourceDataReader> reader, const String& encryptionKey = "", bool preLoad = false);
		~AssetPack();

		AssetPack& operator=(AssetPack&& other);

		AssetDatabase& getAssetDatabase();
		const AssetDatabase& getAssetDatabase() const;
		Bytes& getData();
		const Bytes& getData() const;

		Bytes writeOut() const;

		void readToMemory();
		void encrypt(const String& key);
		void decrypt(const String& key);

    private:
		std::unique_ptr<AssetDatabase> assetDb;
		std::unique_ptr<ResourceDataReader> reader;
		size_t dataOffset = 0;
		Bytes data;
    };
}
