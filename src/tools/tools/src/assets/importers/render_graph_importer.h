#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class RenderGraphImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::RenderGraphDefinition; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
