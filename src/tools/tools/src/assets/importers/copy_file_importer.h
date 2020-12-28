#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class CopyFileImporter : public IAssetImporter
	{
	public:
		CopyFileImporter(ImportAssetType importType, AssetType outputType);
		
		ImportAssetType getType() const override { return importType; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
		int dropFrontCount() const override { return 0; }

	private:
		const ImportAssetType importType;
		const AssetType outputType;
	};
}
