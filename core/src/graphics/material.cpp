#include "../resources/resources.h"
#include "../api/halley_api_internal.h"
#include "../api/halley_api.h"
#include "material.h"
#include "shader.h"
#include "material_parameter.h"
#include <yaml-cpp/yaml.h>

using namespace Halley;

static Material* currentMaterial = nullptr;

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment(lib, "libyaml-cppmdd.lib")
#else
#pragma comment(lib, "libyaml-cppmd.lib")
#endif
#endif

Material::Material(std::shared_ptr<Shader> _shader, VideoAPI* api)
	: api(api)
	, shader(_shader)
{
}

void Material::bind()
{
	// Avoid redundant work
	if (currentMaterial == this && !dirty) {
		return;
	}
	currentMaterial = this;

	if (!shader) {
		throw Exception("Material has no shader.");
	}

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

	shader->bind();

	for (auto& u : uniforms) {
		u.bind();
	}
}

Shader& Material::getShader() const
{
	return *shader;
}

void Material::ensureLoaded()
{
	// TODO?
}

MaterialParameter& Material::operator[](String name)
{
	shader->bind();
	dirty = true;

	for (auto& u : uniforms) {
		if (u.name == name) {
			return u;
		}
	}
	uniforms.push_back(MaterialParameter(*this, name));
	return uniforms.back();
}

Blend::Type Material::getBlend() const
{
	// TODO
	return Blend::Alpha_Premultiplied;
}

std::unique_ptr<Material> Material::loadResource(ResourceLoader& loader)
{
	auto& api = loader.getAPI();

	String basePath = loader.getName();
	size_t lastSlash = basePath.find_last_of('/');
	if (lastSlash != std::string::npos) {
		basePath = basePath.left(lastSlash + 1);
	}

	YAML::Node root = YAML::Load(loader.getStatic()->getString());
	for (auto& passNode : root["passes"]) {
		auto pass = passNode.as<YAML::Node>();

		// Load shader
		auto shader = api.video->createShader(loader.getName());
		auto load = [&] (std::string name, std::function<void(String)> f)
		{
			if (pass[name + "Source"]) {
				f(pass[name + "Source"].as<std::string>());
			} else if (pass[name]) {
				f(api.core->getResources().get<TextFile>(basePath + pass[name].as<std::string>())->data);
			}
		};
		load("vertex", [&](String src) { shader->addVertexSource(src); });
		load("pixel", [&](String src) { shader->addPixelSource(src); });
		load("geometry", [&](String src) { shader->addGeometrySource(src); });
		shader->compile();

		return std::make_unique<Material>(std::move(shader), loader.getAPI().video);
	}

	throw Exception("No passes found in shader.");
}
