#include "resources/resources.h"
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/shader.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <yaml-cpp/yaml.h>
#include <halley/file_formats/text_file.h>

using namespace Halley;

static Material* currentMaterial = nullptr;
static int currentPass = 0;

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "libyaml-cppmdd.lib")
#else
#pragma comment(lib, "libyaml-cppmd.lib")
#endif
#endif

MaterialDefinition::MaterialDefinition(ResourceLoader& loader)
{
	String basePath = loader.getBasePath();

	YAML::Node root;
	try {
		root = YAML::Load(loader.getStatic()->getString());
	}
	catch (std::exception& e) {
		throw Exception("Exception parsing material " + loader.getName() + ": " + e.what());
	}

	// Load name
	name = root["name"] ? root["name"].as<std::string>() : "Unknown";

	// Load attributes & uniforms
	if (root["attributes"]) {
		loadAttributes(root["attributes"]);
	}
	if (root["uniforms"]) {
		loadUniforms(root["uniforms"]);
	}

	// Load passes
	for (auto passNode : root["passes"]) {
		loadPass(passNode.as<YAML::Node>(), [&](String path) {
			return loader.getAPI().getResource<TextFile>("shader/" + path)->data;
		});
	}

	loadShader(loader.getAPI().video);
}

void MaterialDefinition::loadPass(YAML::Node node, std::function<String(String)> retriever)
{
	// Load shader
	auto load = [&](std::string name) -> String
	{
		if (node[name + "Source"]) {
			return node[name + "Source"].as<std::string>();
		}
		else if (node[name]) {
			return retriever(node[name].as<std::string>());
		}
		return "";
	};

	String blend = node["blend"].as<std::string>("Opaque");
	BlendType blendType;
	if (blend == "Opaque") {
		blendType = BlendType::Opaque;
	}
	else if (blend == "Alpha") {
		blendType = BlendType::Alpha;
	}
	else if (blend == "AlphaPremultiplied") {
		blendType = BlendType::AlphaPremultiplied;
	}
	else {
		throw Exception("Unknown blend type: " + blend);
	}

	passes.emplace_back(MaterialPass(blendType, load("vertex"), load("geometry"), load("pixel")));
}

void MaterialDefinition::loadUniforms(YAML::Node topNode)
{
	auto attribSeqNode = topNode.as<YAML::Node>();
	for (auto attribEntry : attribSeqNode) {
		for (YAML::const_iterator it = attribEntry.begin(); it != attribEntry.end(); ++it) {
			String name = it->first.as<std::string>();
			ShaderParameterType type = parseParameterType(it->second.as<std::string>());

			uniforms.push_back(MaterialAttribute(name, type, -1));
		}
	}
}

ShaderParameterType MaterialDefinition::parseParameterType(String rawType)
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

void MaterialDefinition::loadAttributes(YAML::Node topNode)
{
	int location = 0;
	int offset = 0;

	auto attribSeqNode = topNode.as<YAML::Node>();
	for (auto attribEntry : attribSeqNode) {
		for (YAML::const_iterator it = attribEntry.begin(); it != attribEntry.end(); ++it) {
			String name = it->first.as<std::string>();
			ShaderParameterType type = parseParameterType(it->second.as<std::string>());

			attributes.push_back(MaterialAttribute());
			auto& a = attributes.back();
			a.name = name;
			a.type = type;
			a.location = location++;
			a.offset = offset;

			int size = getAttributeSize(type);
			offset += size;

			if (a.name == "a_vertPos") {
				vertexPosOffset = a.offset;
			}
		}
	}

	vertexStride = offset;
}

int MaterialDefinition::getAttributeSize(ShaderParameterType type)
{
	switch (type) {
	case ShaderParameterType::Float: return 4;
	case ShaderParameterType::Float2: return 8;
	case ShaderParameterType::Float3: return 12;
	case ShaderParameterType::Float4: return 16;
	default: throw Exception("Unknown type: " + String::integerToString(int(type)));
	}
}

void MaterialDefinition::bind(int pass, Painter& painter)
{
	passes[pass].bind(painter);
}

int MaterialDefinition::getNumPasses() const
{
	return int(passes.size());
}

MaterialPass& MaterialDefinition::getPass(int n)
{
	return passes[n];
}

std::unique_ptr<MaterialDefinition> MaterialDefinition::loadResource(ResourceLoader& loader)
{
	return std::make_unique<MaterialDefinition>(loader);
}

void MaterialDefinition::loadShader(VideoAPI* _api)
{
	api = _api;
	int i = 0;
	for (auto& p: passes) {
		p.createShader(api, name + "/pass" + String::integerToString(i++), attributes);
	}
}

MaterialPass::MaterialPass(BlendType blend, String vertexSrc, String geometrySrc, String pixelSrc)
	: blend(blend)
	, vertexSrc(vertexSrc)
	, geometrySrc(geometrySrc)
	, pixelSrc(pixelSrc)
{
}

void MaterialPass::bind(Painter& painter) const
{
	shader->bind();
	painter.setBlend(getBlend());
}

void MaterialPass::createShader(VideoAPI* api, String name, const Vector<MaterialAttribute>& attributes)
{
	shader = api->createShader(name);
	shader->setAttributes(attributes);
	if (vertexSrc != "") {
		shader->addVertexSource(vertexSrc);
	}
	if (geometrySrc != "") {
		shader->addGeometrySource(geometrySrc);
	}
	if (pixelSrc != "") {
		shader->addPixelSource(pixelSrc);
	}
	shader->compile();
}
