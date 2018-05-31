#include "material_importer.h"
#include "halley/file/byte_serializer.h"
#include "halley/core/graphics/material/material_definition.h"
#include "../../yaml/halley-yamlcpp.h"
#include "halley/tools/file/filesystem.h"
#include "halley/text/string_converter.h"
#include "config_importer.h"

using namespace Halley;

void MaterialImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Path basePath = asset.inputFiles.at(0).name.parentPath();
	auto material = parseMaterial(basePath, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)), collector);
	collector.output(material.getName(), AssetType::MaterialDefinition, Serializer::toBytes(material));
}

MaterialDefinition MaterialImporter::parseMaterial(Path basePath, gsl::span<const gsl::byte> data, IAssetCollector& collector) const
{
	String strData(reinterpret_cast<const char*>(data.data()), data.size());
	YAML::Node yamlRoot = YAML::Load(strData.cppStr());
	auto root = ConfigImporter::parseYAMLNode(yamlRoot);

	// Load base material
	MaterialDefinition material;
	if (root.hasKey("base")) {
		String baseName = root["base"].asString();
		auto otherData = collector.readAdditionalFile(basePath / baseName);
		material = parseMaterial(basePath, gsl::as_bytes(gsl::span<Byte>(otherData)), collector);
	}
	material.load(root);

	// Load passes
	int passN = 0;
	if (root.hasKey("passes")) {
		for (auto& passNode: root["passes"].asSequence()) {
			loadPass(material, passNode, collector, passN++);
		}
	}

	return material;
}

void MaterialImporter::loadPass(MaterialDefinition& material, const ConfigNode& node, IAssetCollector& collector, int passN)
{
	String passName = material.getName() + "_pass_" + toString(passN);

	auto shaderTypes = { "vertex", "geometry", "pixel" };

	for (auto& shaderEntry: node["shader"]) {
		String language = shaderEntry["language"].asString();
		String shaderName = passName;
		ImportingAsset shaderAsset;
		shaderAsset.assetId = shaderName + ":" + language;
		shaderAsset.assetType = ImportAssetType::Shader;
		for (auto& curType: shaderTypes) {
			if (shaderEntry.hasKey(curType)) {
				auto data = collector.readAdditionalFile("shader/" + shaderEntry[curType].asString());
				Metadata meta;
				meta.set("language", language);
				shaderAsset.inputFiles.emplace_back(ImportingAssetFile(shaderName + "." + curType, std::move(data), meta));
			}
		}
		collector.addAdditionalAsset(std::move(shaderAsset));
	}

	material.addPass(MaterialPass(passName, node));
}
