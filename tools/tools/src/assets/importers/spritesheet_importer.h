#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class SpriteSheetImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::SpriteSheet; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
