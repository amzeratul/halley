#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class AudioEventImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::AudioEvent; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
