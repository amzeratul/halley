#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class TextureImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Texture; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
