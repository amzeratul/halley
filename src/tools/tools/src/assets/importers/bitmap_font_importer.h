#pragma once
#include "halley/plugin/iasset_importer.h"
#include "halley/maths/vector2.h"
#include "halley/core/graphics/text/font.h"

namespace Halley
{
	class BitmapFontImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::BitmapFont; }

		String getAssetId(const Path& file, const std::optional<Metadata>& metadata) const override;

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		Font parseBitmapFontXML(Vector2i imageSize, const Bytes& data);
	};
}
