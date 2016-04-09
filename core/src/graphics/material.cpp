#include "../resources/resources.h"
#include "../api/halley_api_internal.h"
#include "../api/halley_api.h"
#include "material.h"
#include "texture.h"
#include "shader.h"

using namespace Halley;

static Material* currentMaterial = nullptr;

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
	toApply(*this);
}

void MaterialParameter::bind()
{
	toBind(*this);
}

void MaterialParameter::operator=(std::shared_ptr<Texture> texture)
{
	needsTextureUnit = true;
	toApply = [texture](MaterialParameter& p0) {
		p0.toBind = [texture](MaterialParameter& p) {
			texture->bind(p.textureUnit);
			p.getAPI().getUniformBinding(p.getAddress(), UniformType::Int, 1, &p.textureUnit);
		};
	};
}

void MaterialParameter::operator=(Colour colour)
{
	needsTextureUnit = false;
	toApply = [colour](MaterialParameter& p) {
		std::array<float, 4> col = { colour.r, colour.g, colour.b, colour.a };
		p.toBind = p.getAPI().getUniformBinding(p.getAddress(), UniformType::Float, 4, col.data());
	};
}

void MaterialParameter::operator=(float p)
{
	needsTextureUnit = false;
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Float, 1, &v);
	};
}

void MaterialParameter::operator=(Vector2f p)
{
	needsTextureUnit = false;
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Float, 2, &v);
	};
}

void MaterialParameter::operator=(int p)
{
	needsTextureUnit = false;
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Int, 1, &v);
	};
}

void MaterialParameter::operator=(Vector2i p)
{
	needsTextureUnit = false;
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Int, 2, &v);
	};
}

void MaterialParameter::operator=(Matrix4f m)
{
	needsTextureUnit = false;
	toApply = [m](MaterialParameter& t) {
		auto v = m;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Mat4, 1, &v);
	};
}
