#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/material/uniform_type.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/shader.h"
#include "halley/core/graphics/texture.h"
#include <gsl/gsl_assert>

using namespace Halley;

MaterialParameter::MaterialParameter(Material& material, String name, ShaderParameterType type)
	: material(material)
	, name(name)
	, type(type)
{
	needsTextureUnit = type == ShaderParameterType::Texture2D;
}

VideoAPIInternal& MaterialParameter::getAPI() const
{
	return *static_cast<VideoAPIInternal*>(material.getDefinition().api);
}

void MaterialParameter::updateAddresses()
{
	auto& definition = material.getDefinition();
	addresses.resize(definition.passes.size());
	for (size_t i = 0; i < addresses.size(); i++) {
		auto& shader = definition.passes[i].getShader();
		addresses[i] = shader.getUniformLocation(name);
	}
}

unsigned int MaterialParameter::getAddress(int pass)
{
	return addresses[pass];
}

void MaterialParameter::bind(int pass)
{
	if (toBind) {
		toBind(pass);
	}
}

void MaterialParameter::operator=(std::shared_ptr<const Texture> texture)
{
	Expects(texture);
	Expects(type == ShaderParameterType::Texture2D);
	bindFunc = getAPI().getUniformBinding(UniformType::Int, 1);
	toBind = [texture, this](int pass) {
		texture->bind(textureUnit);
		bindFunc(getAddress(pass), &textureUnit);
	};
}

void MaterialParameter::operator=(Colour colour)
{
	Expects(type == ShaderParameterType::Float4);
	bindFunc = getAPI().getUniformBinding(UniformType::Float, 4);
	toBind = [=](int pass) {
		Colour c = colour;
		bindFunc(getAddress(pass), &c);
	};
}

void MaterialParameter::operator=(float p)
{
	Expects(type == ShaderParameterType::Float);
	bindFunc = getAPI().getUniformBinding(UniformType::Float, 1);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(Vector2f p)
{
	Expects(type == ShaderParameterType::Float2);
	bindFunc = getAPI().getUniformBinding(UniformType::Float, 2);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(Vector3f p)
{
	Expects(type == ShaderParameterType::Float3);
	bindFunc = getAPI().getUniformBinding(UniformType::Float, 3);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(Vector4f p)
{
	Expects(type == ShaderParameterType::Float4);
	bindFunc = getAPI().getUniformBinding(UniformType::Float, 4);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(int p)
{
	Expects(type == ShaderParameterType::Int);
	bindFunc = getAPI().getUniformBinding(UniformType::Int, 1);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(Vector2i p)
{
	Expects(type == ShaderParameterType::Int2);
	bindFunc = getAPI().getUniformBinding(UniformType::Int, 2);
	toBind = [=](int pass) {
		auto l = p;
		bindFunc(getAddress(pass), &l);
	};
}

void MaterialParameter::operator=(Matrix4f m)
{
	Expects(type == ShaderParameterType::Matrix4);
	bindFunc = getAPI().getUniformBinding(UniformType::Mat4, 1);
	toBind = [=](int pass) {
		auto l = m;
		bindFunc(getAddress(pass), l.getElements());
	};
}
