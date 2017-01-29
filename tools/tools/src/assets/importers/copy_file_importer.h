#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class CopyFileImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::SimpleCopy; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
