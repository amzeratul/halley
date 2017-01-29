#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class CodegenImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Codegen; }

		String getAssetId(const Path& file) const override;
		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
