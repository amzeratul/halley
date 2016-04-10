#include "material_parameter.h"
#include "uniform_type.h"
#include "material.h"
#include "../api/halley_api.h"
#include "shader.h"
#include "texture.h"

using namespace Halley;

MaterialParameter::MaterialParameter(Material& material, String name, ShaderParameterType type)
	: material(material)
	, name(name)
	, type(type)
{
	needsTextureUnit = type == ShaderParameterType::Texture2D;
}

VideoAPIInternal& MaterialParameter::getAPI()
{
	return *static_cast<VideoAPIInternal*>(material.api);
}

void MaterialParameter::updateAddresses()
{
	addresses.resize(material.passes.size());
	for (size_t i = 0; i < addresses.size(); i++) {
		addresses[i] = material.passes[i].getShader().getUniformLocation(name);
	}
}

unsigned int MaterialParameter::getAddress()
{
	return addresses[0];
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
	assert(type == ShaderParameterType::Texture2D);
	toApply = [texture](MaterialParameter& p0) {
		p0.toBind = [texture](MaterialParameter& p) {
			texture->bind(p.textureUnit);
			p.getAPI().getUniformBinding(p.getAddress(), UniformType::Int, 1, &p.textureUnit)(p);
		};
	};
}

void MaterialParameter::operator=(Colour colour)
{
	assert(type == ShaderParameterType::Float4);
	toApply = [colour](MaterialParameter& p) {
		std::array<float, 4> col = { colour.r, colour.g, colour.b, colour.a };
		p.toBind = p.getAPI().getUniformBinding(p.getAddress(), UniformType::Float, 4, col.data());
	};
}

void MaterialParameter::operator=(float p)
{
	assert(type == ShaderParameterType::Float);
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Float, 1, &v);
	};
}

void MaterialParameter::operator=(Vector2f p)
{
	assert(type == ShaderParameterType::Float2);
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Float, 2, &v);
	};
}

void MaterialParameter::operator=(int p)
{
	assert(type == ShaderParameterType::Int);
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Int, 1, &v);
	};
}

void MaterialParameter::operator=(Vector2i p)
{
	assert(type == ShaderParameterType::Int2);
	toApply = [p](MaterialParameter& t) {
		auto v = p;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Int, 2, &v);
	};
}

void MaterialParameter::operator=(Matrix4f m)
{
	assert(type == ShaderParameterType::Matrix4);
	toApply = [m](MaterialParameter& t) {
		auto v = m;
		t.toBind = t.getAPI().getUniformBinding(t.getAddress(), UniformType::Mat4, 1, &v);
	};
}
