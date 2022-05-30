#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class ScriptGraphImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::ScriptGraph; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
