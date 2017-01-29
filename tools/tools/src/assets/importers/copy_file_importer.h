#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class CopyFileImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::SimpleCopy; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
