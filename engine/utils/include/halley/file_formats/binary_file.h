#pragma once
#include <memory>
#include "halley/resources/resource.h"
#include "halley/utils/utils.h"

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

		Bytes data;

		static std::unique_ptr<BinaryFile> loadResource(ResourceLoader& loader);
	};
}
