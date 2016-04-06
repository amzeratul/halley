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

unsigned int MaterialParameter::getAddress()
{
	return material.shader->getUniformLocation(name);
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
		int id = texture->getNativeId();
		toBind = getAPI().getUniformBinding(getAddress(), UniformType::Int, 1, &id);
	};
}

void MaterialParameter::operator=(Colour colour)
{
	toApply = [=]() {
		std::array<float, 4> col = { colour.r, colour.g, colour.b, colour.a };
		toBind = getAPI().getUniformBinding(getAddress(), UniformType::Float, 4, col.data());
	};
}

void MaterialParameter::operator=(float p)
{
	toApply = [=]() {
		auto v = p;
		toBind = getAPI().getUniformBinding(getAddress(), UniformType::Float, 1, &v);
	};
}

void MaterialParameter::operator=(Vector2f p)
{
	toApply = [=]() {
		auto v = p;
		toBind = getAPI().getUniformBinding(getAddress(), UniformType::Float, 2, &v);
	};
}

void MaterialParameter::operator=(int p)
{
	toApply = [=]() {
		auto v = p;
		toBind = getAPI().getUniformBinding(getAddress(), UniformType::Int, 1, &v);
	};
}

void MaterialParameter::operator=(Vector2i p)
{
	toApply = [=]() {
		auto v = p;
		toBind = getAPI().getUniformBinding(getAddress(), UniformType::Int, 2, &v);
	};
}
