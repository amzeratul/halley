#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class ShaderImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Shader; }

		std::vector<Path> import(const ImportingAsset& asset, const Path& dstDir, ProgressReporter reporter, AssetCollector collector) override;
	};
}
