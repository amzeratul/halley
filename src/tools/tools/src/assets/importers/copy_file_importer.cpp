#include "copy_file_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void CopyFileImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	collector.output(asset.assetId, AssetType::BinaryFile, asset.inputFiles[0].data, asset.inputFiles[0].metadata);
}
