#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/shader.h"
#include <gsl/gsl_assert>

using namespace Halley;

constexpr static int shaderStageCount = int(ShaderType::NumOfShaderTypes);

MaterialTextureParameter::MaterialTextureParameter(Material& material, const String& name, TextureSamplerType samplerType)
	: name(name)
	, samplerType(samplerType)
{
	auto& definition = material.getDefinition();
	addresses.resize(definition.passes.size() * shaderStageCount);
	for (size_t i = 0; i < definition.passes.size(); i++) {
		auto& shader = definition.passes[i].getShader();
		for (int j = 0; j < shaderStageCount; ++j) {
			addresses[i * shaderStageCount + j] = shader.getUniformLocation(name, ShaderType(j));
		}
	}
}

unsigned MaterialTextureParameter::getAddress(int pass, ShaderType stage) const
{
	return addresses[pass * shaderStageCount + int(stage)];
}

MaterialParameter::MaterialParameter(Material& material, String name, ShaderParameterType type, int blockNumber, size_t offset)
	: material(&material)
	, name(std::move(name))
	, offset(offset)
	, type(type)
	, blockNumber(blockNumber)
{
}

void MaterialParameter::rebind(Material& m) noexcept
{
	material = &m;
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
