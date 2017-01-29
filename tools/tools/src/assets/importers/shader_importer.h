#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class ShaderImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Shader; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
