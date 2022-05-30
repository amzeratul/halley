#include "script_graph_importer.h"

#include "halley/entity/scripting/script_graph.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file_formats/config_file.h"
#include "halley/file_formats/yaml_convert.h"

using namespace Halley;

void ScriptGraphImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ConfigFile config = YAMLConvert::parseConfig(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	
	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");

	const auto scriptGraph = ScriptGraph(config.getRoot());

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::ScriptGraph, Serializer::toBytes(scriptGraph, SerializerOptions(SerializerOptions::maxVersion)), meta);
}
