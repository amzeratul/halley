#pragma once
#include <memory>
#include "halley/resources/resource.h"
#include "halley/utils/utils.h"
#include <gsl/gsl>
#include "halley/core/resources/resource_collection.h"

namespace Halley
{
	class ResourceLoader;

	class BinaryFile : public Resource
	{
	public:
		BinaryFile();
		explicit BinaryFile(const Bytes& data);
        explicit BinaryFile(Bytes&& data);
		explicit BinaryFile(gsl::span<const gsl::byte> data);
		explicit BinaryFile(std::unique_ptr<ResourceDataStream> stream);

		static std::unique_ptr<BinaryFile> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::BinaryFile; }
		void reload(Resource&& resource) override;

		const Bytes& getBytes() const;
		Bytes& getBytes();
		gsl::span<const gsl::byte> getSpan() const;
		gsl::span<gsl::byte> getSpan();

		std::shared_ptr<ResourceDataStream> getStream() const;

	private:
		Bytes data;
		std::shared_ptr<ResourceDataStream> stream;
		bool streaming = false;
	};
}
