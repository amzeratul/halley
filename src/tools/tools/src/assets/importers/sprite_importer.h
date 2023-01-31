#pragma once
#include "halley/plugin/iasset_importer.h"
#include "halley/file_formats/image.h"
#include "halley/core/graphics/sprite/sprite_sheet.h"
#include "halley/data_structures/bin_pack.h"

namespace Halley
{
	class Animation;
	class SpriteImporter;
	
	class SpriteImporter : public IAssetImporter
	{
	public:
		using ImageData = SpriteSheet::ImageData;

		ImportAssetType getType() const override { return ImportAssetType::Sprite; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
		String getAssetId(const Path& file, const std::optional<Metadata>& metadata) const override;

	private:
		Animation generateAnimation(const String& spriteName, const String& spriteSheetName, const String& materialName, const Vector<ImageData>& frameData, const String& fallbackSequenceName);

		Vector<ImageData> splitImagesInGrid(const Vector<ImageData>& images, Vector2i grid);
	};
}
