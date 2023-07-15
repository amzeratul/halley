#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class TextureImporter : public IAssetImporter
	{
	public:
		explicit TextureImporter(bool lz4hc);
		ImportAssetType getType() const override { return ImportAssetType::Texture; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		bool lz4hc;
	};
}
