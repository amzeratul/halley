#include "image_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/file_formats/image.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void ImageImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	// Prepare metadata
	Metadata meta;
	if (asset.metadata) {
		meta = *asset.metadata;
	}

	// Load image
	Path mainFile = asset.inputFiles.at(0).name;
	auto span = gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles[0].data));
	std::unique_ptr<Image> image;
	if (meta.getString("compression", "png") == "png") {
		image = std::make_unique<Image>(span, fromString<Image::Mode>(meta.getString("mode", "undefined")));
	} else {
		image = std::make_unique<Image>();
		Deserializer s(span);
		s >> *image;
	}

	// Convert to indexed mode
	auto palette = meta.getString("palette", "");
	if (palette != "") {
		auto paletteBytes = collector.readAdditionalFile(palette);
		Image paletteImage(gsl::as_bytes(gsl::span<Byte>(paletteBytes)));
		image = convertToIndexed(*image, paletteImage);
	}

	// Fill meta
	Vector2i size = image->getSize();
	meta.set("width", size.x);
	meta.set("height", size.y);
	meta.set("mode", toString(image->getMode()));

	// Output
	ImportingAsset imageAsset;
	imageAsset.assetId = asset.assetId;
	imageAsset.assetType = ImportAssetType::Texture;
	imageAsset.metadata = std::make_unique<Metadata>(meta);
	imageAsset.inputFiles.emplace_back(ImportingAssetFile(asset.assetId, Serializer::toBytes(*image)));
	collector.addAdditionalAsset(std::move(imageAsset));
}

std::unique_ptr<Image> ImageImporter::convertToIndexed(const Image& image, const Image& palette)
{
	auto result = std::make_unique<Image>(Image::Mode::Indexed, image.getSize());
	return result;
}
