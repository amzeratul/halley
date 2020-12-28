#include "copy_file_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

CopyFileImporter::CopyFileImporter(ImportAssetType importType, AssetType outputType)
	: importType(importType)
	, outputType(outputType)
{
}

void CopyFileImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	collector.output(asset.assetId, outputType, asset.inputFiles[0].data, asset.inputFiles[0].metadata);
}
