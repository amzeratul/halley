#include "material_importer.h"
#include "halley/bytes/byte_serializer.h"
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
				auto data = loadShader(shaderEntry[curType].asString(), collector);
				Metadata meta;
				meta.set("language", language);
				shaderAsset.inputFiles.emplace_back(ImportingAssetFile(shaderName + "." + curType, std::move(data), meta));
			}
		}
		collector.addAdditionalAsset(std::move(shaderAsset));
	}

	material.addPass(MaterialPass(passName, node));
}

Bytes MaterialImporter::loadShader(const String& name, IAssetCollector& collector)
{
	std::set<String> loaded;
	return doLoadShader(name, collector, loaded);
}

Bytes MaterialImporter::doLoadShader(const String& name, IAssetCollector& collector, std::set<String>& loaded)
{
	Bytes finalResult;
	const auto appendLine = [&] (const void* data, size_t size)
	{
		const size_t curSize = finalResult.size();
		finalResult.resize(curSize + size + 1);
		memcpy(finalResult.data() + curSize, data, size);
		memcpy(finalResult.data() + curSize + size, "\n", 1);
	};

	Bytes rawData = collector.readAdditionalFile("shader/" + name);
	String strData = String(reinterpret_cast<const char*>(rawData.data()), rawData.size());
	auto lines = strData.split('\n');
	for (size_t i = 0; i < lines.size(); ++i) {
		const auto& curLine = lines[i];
		if (curLine.startsWith("#include")) {
			auto words = curLine.split(' ');
			auto quoted = words.at(1).trimBoth();
			if (quoted.startsWith("\"") && quoted.endsWith("\"")) {
				auto includeFile = quoted.mid(1, quoted.size() - 2);
				if (loaded.find(includeFile) == loaded.end()) {
					loaded.insert(includeFile);
					auto includeData = doLoadShader(includeFile, collector, loaded);
					appendLine(includeData.data(), includeData.size());
				}
			} else if (!(quoted.startsWith("<") && quoted.endsWith(">"))) {
				throw Exception("Invalid syntax in #include in shader", HalleyExceptions::Tools);
			} else {
				appendLine(curLine.c_str(), curLine.size());
			}
		} else {
			appendLine(curLine.c_str(), curLine.size());
		}
	}

	return finalResult;
}
