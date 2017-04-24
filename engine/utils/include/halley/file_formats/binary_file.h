#pragma once
#include <memory>
#include "halley/resources/resource.h"
#include "halley/utils/utils.h"
#include <gsl/gsl>

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

		static std::unique_ptr<BinaryFile> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::BinaryFile; }

		const Bytes& getBytes() const;
		Bytes& getBytes();
		gsl::span<const gsl::byte> getSpan() const;
		gsl::span<gsl::byte> getSpan();

	private:
		Bytes data;
	};
}
