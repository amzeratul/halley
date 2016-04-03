#include "../resources/resources.h"
#include "../api/halley_api_internal.h"
#include "../api/halley_api.h"
#include "material.h"
#include "texture.h"
#include "shader.h"

using namespace Halley;

Material::Material(std::shared_ptr<Shader> _shader, VideoAPI* api)
	: api(api)
	, shader(_shader)
{
}

void Material::bind()
{
	if (!shader) {
		throw Exception("Material has no shader.");
	}

	if (dirty) {
		for (auto& u : uniforms) {
			u.apply();
		}
		dirty = false;
	}

	shader->bind();

	for (auto& u : uniforms) {
		u.bind();
	}
}

void Material::ensureLoaded()
{
	// TODO?
}

MaterialParameter& Material::operator[](String name)
{
	dirty = true;
	bind();

	for (auto& u : uniforms) {
		if (u.name == name) {
			return u;
		}
	}
	uniforms.push_back(MaterialParameter(*this, name));
	return uniforms.back();
}

std::unique_ptr<Material> Material::loadResource(ResourceLoader loader)
{
	// TODO: read shader
	// TODO: read parameters
	return std::make_unique<Material>(std::shared_ptr<Shader>(), loader.getAPI().video);
}

MaterialParameter::MaterialParameter(Material& material, String name)
	: material(material)
	, name(name)
{
}

VideoAPIInternal& MaterialParameter::getAPI()
{
	return *static_cast<VideoAPIInternal*>(material.api);
}

void MaterialParameter::apply()
{
	toApply();
}

void MaterialParameter::bind()
{
	toBind();
}

void MaterialParameter::operator=(std::shared_ptr<Texture> texture)
{
	toApply = [=]() {
		auto address = material.shader->getUniformLocation(name);
		int id = texture->getNativeId();
		toBind = getAPI().getUniformBinding(address, UniformType::Int, 1, &id);
	};
}
