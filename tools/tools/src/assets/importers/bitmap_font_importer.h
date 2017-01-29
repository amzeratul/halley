#pragma once
#include "halley/tools/assets/iasset_importer.h"
#include "halley/maths/vector2.h"

namespace Halley
{
	class BitmapFontImporter : public IAssetImporter
	{
	public:
		AssetType getType() const override { return AssetType::BitmapFont; }

		String getAssetId(const Path& file) const override;

		std::vector<Path> import(const ImportingAsset& asset, const Path& dstDir, ProgressReporter reporter, AssetCollector collector) override;

	private:
		void parseBitmapFontXML(String imageName, Vector2i imageSize, const Bytes& data, const Path& path);
	};
}
