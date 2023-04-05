#include "game_properties_importer.h"

#include "halley/properties/game_properties.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file_formats/config_file.h"
#include "halley/file_formats/yaml_convert.h"

using namespace Halley;

void GamePropertiesImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ConfigFile config = YAMLConvert::parseConfig(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	
	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "lz4");

	auto properties = GameProperties(config.getRoot());

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::GameProperties, Serializer::toBytes(properties), meta);
}
