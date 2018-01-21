#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class FontImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Font; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
