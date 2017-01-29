#pragma once
#include "halley/plugin/iasset_importer.h"
#include "halley/maths/vector2.h"

namespace Halley
{
	class BitmapFontImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::BitmapFont; }

		String getAssetId(const Path& file) const override;

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		Bytes parseBitmapFontXML(String imageName, Vector2i imageSize, const Bytes& data);
	};
}
