#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class FontImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Font; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
