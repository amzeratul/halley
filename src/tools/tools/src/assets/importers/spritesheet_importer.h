#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class SpriteSheetImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::SpriteSheet; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
