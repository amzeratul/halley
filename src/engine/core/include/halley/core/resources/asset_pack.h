#pragma once
#include "halley/utils/utils.h"
#include "halley/text/halleystring.h"
#include <memory>

namespace Halley {
	class Deserializer;
	class Serializer;
	class AssetDatabase;
	class ResourceDataReader;

    class AssetPack {
    public:
		AssetPack();
		AssetPack(AssetPack&& other);
		AssetPack(std::unique_ptr<ResourceDataReader> reader, const String& encryptionKey = "", bool readToMemory = false);
		~AssetPack();

		AssetPack& operator=(AssetPack&& other);

		AssetDatabase& getAssetDatabase();
		const AssetDatabase& getAssetDatabase() const;
		Bytes& getData();
		const Bytes& getData() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
		void writeHeader(Serializer& s) const;
		void readHeader(Deserializer& s);

    private:
		std::unique_ptr<AssetDatabase> assetDb;
		std::unique_ptr<ResourceDataReader> reader;
		size_t dataOffset = 0;
		Bytes data;
    };
}
