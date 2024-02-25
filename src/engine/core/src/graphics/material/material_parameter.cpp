#include "halley/graphics/material/material_parameter.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/api/halley_api.h"
#include "halley/graphics/shader.h"
#include <gsl/assert>

using namespace Halley;

MaterialParameter::MaterialParameter(Material& material, ShaderParameterType type, uint16_t blockNumber, uint32_t offset)
	: material(&material)
	, offset(offset)
	, blockNumber(blockNumber)
	, type(type)
{
}

bool MaterialParameter::set(Colour colour)
{
	Expects(type == ShaderParameterType::Float4);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float4, &colour);
}

bool MaterialParameter::set(float p)
{
	Expects(type == ShaderParameterType::Float);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float, &p);
}

bool MaterialParameter::set(Vector2f p)
{
	Expects(type == ShaderParameterType::Float2);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float2, &p);
}

bool MaterialParameter::set(Vector3f p)
{
	Expects(type == ShaderParameterType::Float3);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float3, &p);
}

bool MaterialParameter::set(Vector4f p)
{
	Expects(type == ShaderParameterType::Float4);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float4, &p);
}

bool MaterialParameter::set(int p)
{
	Expects(type == ShaderParameterType::Int);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Int, &p);
}

bool MaterialParameter::set(Vector2i p)
{
	Expects(type == ShaderParameterType::Int2);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Int2, &p);
}

bool MaterialParameter::set(Vector3i p)
{
	Expects(type == ShaderParameterType::Int3);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Int3, &p);	
}

bool MaterialParameter::set(Vector4i p)
{
	Expects(type == ShaderParameterType::Int4);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Int4, &p);	
}

bool MaterialParameter::set(uint32_t p)
{
	Expects(type == ShaderParameterType::UInt);
	return material->setUniform(blockNumber, offset, ShaderParameterType::UInt, &p);
}

bool MaterialParameter::set(const Matrix4f& m)
{
	Expects(type == ShaderParameterType::Matrix4);
	if (material->getDefinition().isColumnMajor()) {
		auto transposed = m;
		transposed.transpose();
		return material->setUniform(blockNumber, offset, ShaderParameterType::Matrix4, &transposed);
	} else {
		return material->setUniform(blockNumber, offset, ShaderParameterType::Matrix4, &m);
	}
}
