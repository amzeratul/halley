#pragma once
#include "halley/graphics/mesh/mesh.h"

namespace Halley
{
    class ImportingAsset;

    class GLTFReader
	{
	public:
		std::unique_ptr<Mesh> parse(const ImportingAsset& asset, const Bytes& data);

	private:

	};
}
