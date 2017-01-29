#include "copy_file_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/resources/metadata.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

void CopyFileImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Path mainFile = asset.inputFiles.at(0).name;
	collector.output(mainFile, asset.inputFiles[0].data);

	if (asset.metadata) {
		Path metaPath = mainFile.replaceExtension(mainFile.getExtension() + ".meta");
		collector.output(metaPath, Serializer::toBytes(*asset.metadata));
	}
}
