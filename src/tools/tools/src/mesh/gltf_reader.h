#pragma once
#include "halley/graphics/mesh/mesh.h"

namespace Halley
{
    class IAssetCollector;
    class ImportingAsset;

    class GLTFReader
	{
	public:
		std::unique_ptr<Mesh> parseBinary(const Path& path, const Bytes& data, const Metadata& metadata);
		std::unique_ptr<Mesh> parseASCII(const Path& path, const Bytes& binData, const Bytes& gltfData, const Metadata& metadata);

	private:

	};
}
