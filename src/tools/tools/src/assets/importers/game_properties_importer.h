#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class GamePropertiesImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::GameProperties; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
