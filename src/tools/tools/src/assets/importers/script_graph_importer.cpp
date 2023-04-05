#include "script_graph_importer.h"

#include "halley/scripting/script_graph.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/file_formats/config_file.h"
#include "halley/file_formats/yaml_convert.h"
#include "halley/support/logger.h"

using namespace Halley;

void ScriptGraphImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Metadata meta = asset.inputFiles.at(0).metadata;
	meta.set("asset_compression", "lz4");
	
	const auto scriptGraph = loadScript(asset.assetId, asset.inputFiles.at(0).data, collector);

	collector.output(Path(asset.assetId).replaceExtension("").string(), AssetType::ScriptGraph, Serializer::toBytes(scriptGraph, SerializerOptions(SerializerOptions::maxVersion)), meta);
}

void ScriptGraphImporter::loadScriptDependencies(const String& assetId, ScriptGraph& graph, IAssetCollector& collector) const
{
	// NB: Call to appendGraph will append elements to nodes
	auto& nodes = graph.getNodes();
	for (size_t i = 0; i < nodes.size(); ++i) {
		auto& node = nodes[i];
		if (node.getType() == "callExternal") {
			const auto function = node.getSettings()["function"].asString("");
			if (!function.isEmpty()) {
				auto functionData = collector.readAdditionalFile("comet/" + function + ".comet");
				if (functionData.empty()) {
					Logger::logError("Script \"" + function + "\" referenced by " + assetId + " doesn't exist.");
				} else {
					ScriptGraph otherGraph = loadScript(function, functionData, collector);
					graph.appendGraph(static_cast<GraphNodeId>(i), otherGraph);
				}
			}
		}
	}
}

ScriptGraph ScriptGraphImporter::loadScript(const String& assetId, const Bytes& bytes, IAssetCollector& collector) const
{
	ConfigFile config = YAMLConvert::parseConfig(gsl::as_bytes(gsl::span<const Byte>(bytes)));
	auto script = ScriptGraph(config.getRoot());
	script.setAssetId(assetId);
	loadScriptDependencies(assetId, script, collector);
	return script;
}
