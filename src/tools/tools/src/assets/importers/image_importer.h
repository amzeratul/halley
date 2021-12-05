#pragma once
#include "halley/plugin/iasset_importer.h"
#include "halley/file_formats/image.h"

namespace Halley
{
	class ImageImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Image; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		static std::unique_ptr<Image> convertToIndexed(const String& fileName, const Image& image, const Image& palette, const ConfigNode& assetOptions, bool readTopLineOnly);
		static HashMap<uint32_t, uint32_t> makePaletteConversion(const Image& palette, bool readTopLineOnly);
		static std::pair<String, Vector2i> lookupSpritePosition(const String& fileName, Vector2i pos, const ConfigNode& options);
	};
}
