#include "material_importer.h"
#include "halley/file/byte_serializer.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/support/exception.h"
#include <yaml-cpp/yaml.h>
#include "halley/tools/file/filesystem.h"
#include "halley/text/string_converter.h"

using namespace Halley;

void MaterialImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Path basePath = asset.inputFiles.at(0).name.parentPath();
	auto material = parseMaterial(basePath, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)), collector);
	collector.output(material.getName(), AssetType::MaterialDefinition, Serializer::toBytes(material));
}

MaterialDefinition MaterialImporter::parseMaterial(Path basePath, gsl::span<const gsl::byte> data, IAssetCollector& collector) const
{
	MaterialDefinition material;
	String strData(reinterpret_cast<const char*>(data.data()), data.size());
	YAML::Node root = YAML::Load(strData.cppStr());

	// Load base material
	if (root["base"]) {
		String baseName = root["base"].as<std::string>();
		auto otherData = collector.readAdditionalFile(basePath / baseName);
		material = parseMaterial(basePath, gsl::as_bytes(gsl::span<Byte>(otherData)), collector);
	}

	// Load name
	material.name = root["name"] ? root["name"].as<std::string>() : "Unknown";

	// Load attributes & uniforms
	if (root["attributes"]) {
		loadAttributes(material, root["attributes"]);
	}
	if (root["uniforms"]) {
		loadUniforms(material, root["uniforms"]);
	}
	if (root["textures"]) {
		loadTextures(material, root["textures"]);
	}

	// Load passes
	int passN = 0;
	for (auto passNode : root["passes"]) {
		loadPass(material, passNode.as<YAML::Node>(), collector, passN++);
	}

	return material;
}

void MaterialImporter::loadPass(MaterialDefinition& material, const YAML::Node& node, IAssetCollector& collector, int passN)
{
	String blend = node["blend"].as<std::string>("Opaque");
	BlendType blendType;
	if (blend == "Opaque") {
		blendType = BlendType::Opaque;
	} else if (blend == "Alpha") {
		blendType = BlendType::Alpha;
	} else if (blend == "AlphaPremultiplied") {
		blendType = BlendType::AlphaPremultiplied;
	} else if (blend == "Add") {
		blendType = BlendType::Add;
	} else {
		throw Exception("Unknown blend type: " + blend);
	}

	auto shaderType = { "vertex", "geometry", "pixel" };

	String shaderName = material.getName() + "_pass_" + toString(passN);

	ImportingAsset shaderAsset;
	shaderAsset.assetId = shaderName;
	shaderAsset.assetType = ImportAssetType::Shader;
	for (auto& curType: shaderType) {
		if (node[curType].IsDefined()) {
			auto data = collector.readAdditionalFile("shader/" + node[curType].as<std::string>());
			shaderAsset.inputFiles.emplace_back(ImportingAssetFile(shaderName + "." + curType, std::move(data)));
		}
	}
	collector.addAdditionalAsset(std::move(shaderAsset));

	material.passes.emplace_back(MaterialPass(blendType, shaderName));
}

void MaterialImporter::loadUniforms(MaterialDefinition& material, const YAML::Node& topNode)
{
	auto attribSeqNode = topNode.as<YAML::Node>();
	for (auto attribEntry : attribSeqNode) {
		for (YAML::const_iterator it = attribEntry.begin(); it != attribEntry.end(); ++it) {
			String blockName = it->first.as<std::string>();
			auto uniformNodes = it->second.as<YAML::Node>();

			std::vector<MaterialUniform> uniforms;
			for (auto uniformEntry: uniformNodes) {
				for (YAML::const_iterator uit = uniformEntry.begin(); uit != uniformEntry.end(); ++uit) {
					String uniformName = uit->first.as<std::string>();
					ShaderParameterType type = parseParameterType(uit->second.as<std::string>());
					uniforms.push_back(MaterialUniform(uniformName, type));
				}
			}
			
			material.uniformBlocks.push_back(MaterialUniformBlock(blockName, uniforms));
		}
	}
}

void MaterialImporter::loadTextures(MaterialDefinition& material, const YAML::Node& topNode)
{
	auto attribSeqNode = topNode.as<YAML::Node>();
	for (auto attribEntry : attribSeqNode) {
		for (YAML::const_iterator it = attribEntry.begin(); it != attribEntry.end(); ++it) {
			String name = it->first.as<std::string>();
			ShaderParameterType type = parseParameterType(it->second.as<std::string>());
			if (type != ShaderParameterType::Texture2D) {
				throw Exception("Texture \"" + name + "\" must be sampler2D");
			}

			material.textures.push_back(name);
		}
	}
}

ShaderParameterType MaterialImporter::parseParameterType(String rawType)
{
	if (rawType == "float") {
		return ShaderParameterType::Float;
	}
	else if (rawType == "vec2") {
		return ShaderParameterType::Float2;
	}
	else if (rawType == "vec3") {
		return ShaderParameterType::Float3;
	}
	else if (rawType == "vec4") {
		return ShaderParameterType::Float4;
	}
	else if (rawType == "mat4") {
		return ShaderParameterType::Matrix4;
	}
	else if (rawType == "sampler2D") {
		return ShaderParameterType::Texture2D;
	}
	else {
		throw Exception("Unknown attribute type: " + rawType);
	}
}

void MaterialImporter::loadAttributes(MaterialDefinition& material, const YAML::Node& topNode)
{
	int location = int(material.attributes.size());
	int offset = material.vertexStride;

	auto attribSeqNode = topNode.as<YAML::Node>();
	for (auto attribEntry : attribSeqNode) {
		for (YAML::const_iterator it = attribEntry.begin(); it != attribEntry.end(); ++it) {
			String name = it->first.as<std::string>();
			ShaderParameterType type = parseParameterType(it->second.as<std::string>());

			material.attributes.push_back(MaterialAttribute());
			auto& a = material.attributes.back();
			a.name = name;
			a.type = type;
			a.location = location++;
			a.offset = offset;

			int size = int(MaterialAttribute::getAttributeSize(type));
			offset += size;

			if (a.name == "a_vertPos") {
				material.vertexPosOffset = a.offset;
			}
		}
	}

	material.vertexStride = offset;
}
