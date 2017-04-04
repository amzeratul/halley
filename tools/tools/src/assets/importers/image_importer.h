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
		std::unique_ptr<Image> convertToIndexed(const Image& image, const Image& palette);
	};
}
