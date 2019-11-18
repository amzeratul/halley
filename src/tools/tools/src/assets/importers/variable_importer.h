#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class VariableImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::VariableTable; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
