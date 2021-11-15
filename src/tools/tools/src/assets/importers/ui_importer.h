#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class UIImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::UIDefinition; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
