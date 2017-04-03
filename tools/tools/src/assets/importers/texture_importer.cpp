#include "texture_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void TextureImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	// Load image
	Path mainFile = asset.inputFiles.at(0).name;
	auto span = gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles[0].data));
	collector.output(asset.assetId, AssetType::Texture, span, *asset.metadata);
}
