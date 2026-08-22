#pragma once
#include "halley/utils/utils.h"
#include "halley/text/halleystring.h"
#include <memory>
#include <gsl/span>
#include "halley/resources/resource_data.h"
#include "halley/utils/encrypt.h"

namespace Halley {
	enum class AssetType;
	class Deserializer;
	class Serializer;
	class AssetDatabase;
	class ResourceData;
	class ResourceDataReader;

	struct AssetPackHeaderV1 {
		constexpr static char id[8] = { 'H', 'A', 'L', 'L', 'E', 'Y', 'P', 'K' };

		std::array<uint8_t, 16> iv;
		uint64_t assetDbStartPos;
		uint64_t dataStartPos;

		void init(size_t assetDbSize);
	};

	struct AssetPackHeaderV2 {
		constexpr static char id[8] = { 'H', 'A', 'L', 'L', 'E', 'Y', 'P', '2' };

		uint8_t version;
		uint8_t assetDbPadding;
		std::array<char, 6> _padding;
		uint64_t assetDbStartPos;
		uint64_t dataStartPos;
		std::array<uint8_t, 16> iv;

		void init(size_t assetDbSize);
	};

    class AssetPack {
    public:
		AssetPack();
		AssetPack(const AssetPack& other) = delete;
		AssetPack(AssetPack&& other) noexcept;
		AssetPack(std::unique_ptr<ResourceDataReader> reader, std::optional<Encrypt::AESKey> encryptionKey, bool preLoad = false);
		~AssetPack();

		AssetPack& operator=(const AssetPack& other) = delete;
		AssetPack& operator=(AssetPack&& other) noexcept;

		AssetDatabase& getAssetDatabase();
		const AssetDatabase& getAssetDatabase() const;
		Bytes& getData();
		const Bytes& getData() const;

		Bytes writeOut() const;

		std::unique_ptr<ResourceData> getData(const String& asset, AssetType type, bool stream);

		void readToMemory();
		void encrypt(Encrypt::AESKey key);
		void decrypt(Encrypt::AESKey key);
	    
    	void readData(size_t pos, gsl::span<std::byte> dst);

		std::unique_ptr<ResourceDataReader> extractReader();

		std::shared_ptr<bool> getAliveToken() const;

		size_t getMemoryUsage() const;

    private:
		std::unique_ptr<AssetDatabase> assetDb;
		std::unique_ptr<ResourceDataReader> reader;
		size_t dataOffset = 0;
		Bytes data;
		std::array<uint8_t, 16> iv;
		mutable std::shared_ptr<bool> aliveToken;
    };


	class PackDataReader final : public ResourceDataReader {
	public:
		PackDataReader(AssetPack& pack, size_t startPos, size_t fileSize);

		size_t size() const override;
		int read(gsl::span<std::byte> dst) override;
		int readAt(gsl::span<std::byte> dst, size_t pos) override;
		void seek(int64_t pos, int whence) override;
		size_t tell() const override;
		void close() override;
		bool isAvailable() const override;

	private:
		AssetPack& pack;
		const size_t startPos;
		const size_t fileSize;
		std::atomic<size_t> curPos;
		std::shared_ptr<bool> aliveToken;
	};
}
