#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class ImageImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::Image; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
