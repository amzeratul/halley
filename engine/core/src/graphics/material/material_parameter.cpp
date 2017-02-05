#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/material/uniform_type.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/shader.h"
#include "halley/core/graphics/texture.h"
#include <gsl/gsl_assert>

using namespace Halley;

MaterialParameter::MaterialParameter(Material& material, String name, ShaderParameterType type, size_t offset)
	: material(material)
	, name(name)
	, type(type)
	, offset(offset)
{
}

void MaterialParameter::init()
{
	auto& definition = material.getDefinition();
	addresses.resize(definition.passes.size());
	for (size_t i = 0; i < addresses.size(); i++) {
		auto& shader = definition.passes[i].getShader();
		addresses[i] = shader.getUniformLocation(name);
	}
}

unsigned int MaterialParameter::getAddress(int pass) const
{
	return addresses[pass];
}

void MaterialParameter::operator=(std::shared_ptr<const Texture> texture)
{
	Expects(texture);
	Expects(type == ShaderParameterType::Texture2D);
	material.setUniform(offset, ShaderParameterType::Int, &textureUnit);
	material.setTexture(textureUnit, texture);
}

void MaterialParameter::operator=(Colour colour)
{
	Expects(type == ShaderParameterType::Float4);
	material.setUniform(offset, ShaderParameterType::Float4, &colour);
}

void MaterialParameter::operator=(float p)
{
	Expects(type == ShaderParameterType::Float);
	material.setUniform(offset, ShaderParameterType::Float, &p);
}

void MaterialParameter::operator=(Vector2f p)
{
	Expects(type == ShaderParameterType::Float2);
	material.setUniform(offset, ShaderParameterType::Float2, &p);
}

void MaterialParameter::operator=(Vector3f p)
{
	Expects(type == ShaderParameterType::Float3);
	material.setUniform(offset, ShaderParameterType::Float3, &p);
}

void MaterialParameter::operator=(Vector4f p)
{
	Expects(type == ShaderParameterType::Float4);
	material.setUniform(offset, ShaderParameterType::Float4, &p);
}

void MaterialParameter::operator=(int p)
{
	Expects(type == ShaderParameterType::Int);
	material.setUniform(offset, ShaderParameterType::Int, &p);
}

void MaterialParameter::operator=(Vector2i p)
{
	Expects(type == ShaderParameterType::Int2);
	material.setUniform(offset, ShaderParameterType::Int2, &p);
}

void MaterialParameter::operator=(Matrix4f m)
{
	Expects(type == ShaderParameterType::Matrix4);
	material.setUniform(offset, ShaderParameterType::Matrix4, &m);
}

ShaderParameterType MaterialParameter::getType() const
{
	return type;
}

const void* MaterialParameter::getData() const
{
	return material.getData().data() + offset;
}

const String& MaterialParameter::getName() const
{
	return name;
}
