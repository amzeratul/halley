#include "variable_importer.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file_formats/config_file.h"
#include "halley/tools/yaml/yaml_convert.h"
#include "halley/utils/variable.h"

using namespace Halley;

void VariableImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ConfigFile config;
	YAMLConvert::parseConfig(config, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	
	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");

	auto variableTable = VariableTable(config.getRoot());

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::VariableTable, Serializer::toBytes(variableTable), meta);
}
