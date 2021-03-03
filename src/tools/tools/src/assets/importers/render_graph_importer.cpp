#include "render_graph_importer.h"
#include "halley/core/graphics/render_target/render_graph_definition.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file_formats/config_file.h"
#include "halley/file_formats/yaml_convert.h"

using namespace Halley;

void RenderGraphImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	ConfigFile config = YAMLConvert::parseConfig(gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)));
	
	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "deflate");

	auto renderGraph = RenderGraphDefinition(config.getRoot());

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::RenderGraphDefinition, Serializer::toBytes(renderGraph), meta);
}
