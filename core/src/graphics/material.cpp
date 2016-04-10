#include "../resources/resources.h"
#include "../api/halley_api_internal.h"
#include "../api/halley_api.h"
#include "material.h"
#include "shader.h"
#include "material_parameter.h"
#include <yaml-cpp/yaml.h>

using namespace Halley;

static Material* currentMaterial = nullptr;
static size_t currentPass = 0;

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "libyaml-cppmdd.lib")
#else
#pragma comment(lib, "libyaml-cppmd.lib")
#endif
#endif

Material::Material(ResourceLoader& loader)
	: api(loader.getAPI().video)
{
	auto& api = loader.getAPI();

	String basePath = loader.getName();
	size_t lastSlash = basePath.find_last_of('/');
	if (lastSlash != std::string::npos) {
		basePath = basePath.left(lastSlash + 1);
	}

	YAML::Node root;
	try {
		root = YAML::Load(loader.getStatic()->getString());
	} catch (std::exception& e) {
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
	for (auto& passNode : root["passes"]) {
		loadPass(passNode.as<YAML::Node>(), [&] (String path) {
			return api.core->getResources().get<TextFile>(basePath + path)->data;
		});
	}

	// Update uniforms
	for (auto& u : uniforms) {
		u.updateAddresses();
	}
}

void Material::loadPass(YAML::Node node, std::function<String(String)> retriever)
{
	// Load shader
	auto shader = api->createShader(name + "/pass" + String::integerToString(int(passes.size())));
	auto load = [&](std::string name, std::function<void(String)> f)
	{
		if (node[name + "Source"]) {
			f(node[name + "Source"].as<std::string>());
		} else if (node[name]) {
			f(retriever(node[name].as<std::string>()));
		}
	};
	shader->setAttributes(attributes);
	load("vertex", [&](String src) { shader->addVertexSource(src); });
	load("pixel", [&](String src) { shader->addPixelSource(src); });
	load("geometry", [&](String src) { shader->addGeometrySource(src); });
	shader->compile();

	passes.emplace_back(MaterialPass(std::move(shader), Blend::AlphaPremultiplied));	
}

void Material::loadUniforms(YAML::Node topNode)
{
	auto attribSeqNode = topNode.as<YAML::Node>();
	for (auto& attribEntry : attribSeqNode) {
		for (YAML::const_iterator it = attribEntry.begin(); it != attribEntry.end(); ++it) {
			String name = it->first.as<std::string>();
			ShaderParameterType type = parseParameterType(it->second.as<std::string>());

			uniforms.push_back(MaterialParameter(*this, name, type));
		}
	}
}

ShaderParameterType Material::parseParameterType(String rawType)
{
	if (rawType == "float") {
		return ShaderParameterType::Float;
	} if (rawType == "vec2") {
		return ShaderParameterType::Float2;
	} else if (rawType == "vec3") {
		return ShaderParameterType::Float3;
	} else if (rawType == "vec4") {
		return ShaderParameterType::Float4;
	} else if (rawType == "mat4") {
		return ShaderParameterType::Matrix4;
	} else if (rawType == "sampler2D") {
		return ShaderParameterType::Texture2D;
	} else {
		throw Exception("Unknown attribute type: " + rawType);
	}
}

void Material::loadAttributes(YAML::Node topNode)
{
	int location = 0;
	int offset = 0;

	auto attribSeqNode = topNode.as<YAML::Node>();
	for (auto& attribEntry : attribSeqNode) {
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
		}
	}

	vertexStride = offset;
}

int Material::getAttributeSize(ShaderParameterType type)
{
	switch (type) {
	case ShaderParameterType::Float: return 4;
	case ShaderParameterType::Float2: return 8;
	case ShaderParameterType::Float3: return 12;
	case ShaderParameterType::Float4: return 16;
	default: throw Exception("Unknown type: " + String::integerToString(int(type)));
	}
}

void Material::bind(size_t pass)
{
	// Avoid redundant work
	if (currentMaterial == this && currentPass == pass && !dirty) {
		return;
	}
	currentMaterial = this;
	currentPass = pass;

	if (dirty) {
		int tu = 0;
		for (auto& u : uniforms) {
			if (u.needsTextureUnit) {
				u.textureUnit = tu++;
			} else {
				u.textureUnit = -1;
			}
			u.apply();
		}
		dirty = false;
	}

	passes[pass].bind();

	for (auto& u : uniforms) {
		u.bind();
	}
}

size_t Material::getNumPasses() const
{
	return passes.size();
}

MaterialPass& Material::getPass(size_t n)
{
	return passes[n];
}

void Material::ensureLoaded()
{
	// TODO?
}

MaterialParameter& Material::operator[](String name)
{
	passes[0].bind();
	dirty = true;

	for (auto& u : uniforms) {
		if (u.name == name) {
			return u;
		}
	}
	throw Exception("Uniform not available: " + name);
}

std::unique_ptr<Material> Material::loadResource(ResourceLoader& loader)
{
	return std::make_unique<Material>(loader);
}

MaterialPass::MaterialPass(std::shared_ptr<Shader> shader, Blend::Type blend)
	: shader(shader)
	, blend(blend)
{
}

void MaterialPass::bind()
{
	shader->bind();
}
