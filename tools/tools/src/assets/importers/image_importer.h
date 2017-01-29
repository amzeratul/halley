#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class ImageImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Image; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
