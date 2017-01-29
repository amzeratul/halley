#include "copy_file_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void CopyFileImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Path mainFile = asset.assetId;
	if (asset.metadata) {
		collector.output(AssetType::BinaryFile, asset.inputFiles[0].data, *asset.metadata);
	} else {
		collector.output(AssetType::BinaryFile, asset.inputFiles[0].data);
	}
}
