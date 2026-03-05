#include "halley/graphics/material/material_parameter.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/api/halley_api.h"
#include "halley/graphics/shader.h"
#include "halley/support/assert.h"

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
	HalleyAssertDev(type == ShaderParameterType::Float4);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float4, &colour);
}

bool MaterialParameter::set(float p)
{
	HalleyAssertDev(type == ShaderParameterType::Float);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float, &p);
}

bool MaterialParameter::set(Vector2f p)
{
	HalleyAssertDev(type == ShaderParameterType::Float2);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float2, &p);
}

bool MaterialParameter::set(Vector3f p)
{
	HalleyAssertDev(type == ShaderParameterType::Float3);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float3, &p);
}

bool MaterialParameter::set(Vector4f p)
{
	HalleyAssertDev(type == ShaderParameterType::Float4);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Float4, &p);
}

bool MaterialParameter::set(int p)
{
	HalleyAssertDev(type == ShaderParameterType::Int);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Int, &p);
}

bool MaterialParameter::set(Vector2i p)
{
	HalleyAssertDev(type == ShaderParameterType::Int2);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Int2, &p);
}

bool MaterialParameter::set(Vector3i p)
{
	HalleyAssertDev(type == ShaderParameterType::Int3);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Int3, &p);	
}

bool MaterialParameter::set(Vector4i p)
{
	HalleyAssertDev(type == ShaderParameterType::Int4);
	return material->setUniform(blockNumber, offset, ShaderParameterType::Int4, &p);	
}

bool MaterialParameter::set(uint32_t p)
{
	HalleyAssertDev(type == ShaderParameterType::UInt);
	return material->setUniform(blockNumber, offset, ShaderParameterType::UInt, &p);
}

bool MaterialParameter::set(const Matrix4f& m)
{
	HalleyAssertDev(type == ShaderParameterType::Matrix4);
	if (material->getDefinition().isColumnMajor()) {
		auto transposed = m;
		transposed.transpose();
		return material->setUniform(blockNumber, offset, ShaderParameterType::Matrix4, &transposed);
	} else {
		return material->setUniform(blockNumber, offset, ShaderParameterType::Matrix4, &m);
	}
}

bool MaterialParameter::set(const ConfigNode& node)
{
	switch (type) {
	case ShaderParameterType::Float:
		return set(node.asFloat({}));
	case ShaderParameterType::Float2:
		return set(node.asVector2f({}));
	case ShaderParameterType::Float3:
		return set(node.asVector3f({}));
	case ShaderParameterType::Float4:
		return set(node.asVector4f({}));
	case ShaderParameterType::Int:
		return set(node.asInt({}));
	case ShaderParameterType::Int2:
		return set(node.asVector2i({}));
	case ShaderParameterType::Int3:
		return set(node.asVector3i({}));
	case ShaderParameterType::Int4:
		return set(node.asVector4i({}));
	case ShaderParameterType::Matrix2:
		return false;
	case ShaderParameterType::Matrix3:
		return false;
	case ShaderParameterType::Matrix4:
		{
			auto elems = node.asVector<float>({});
			return set(Matrix4f(elems.const_span()));
		}
	case ShaderParameterType::UInt:
		return set(node.asInt({}));
	}
	return false;
}

ConstMaterialParameter::ConstMaterialParameter(const Material& material, ShaderParameterType type, uint16_t blockNumber, uint32_t offset)
	: material(&material)
	, offset(offset)
	, blockNumber(blockNumber)
	, type(type)
{}

bool ConstMaterialParameter::isEqual(Colour colour)
{
	HalleyAssertDev(type == ShaderParameterType::Float4);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Float4, &colour);
}

bool ConstMaterialParameter::isEqual(float p)
{
	HalleyAssertDev(type == ShaderParameterType::Float);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Float, &p);
}

bool ConstMaterialParameter::isEqual(Vector2f p)
{
	HalleyAssertDev(type == ShaderParameterType::Float2);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Float2, &p);
}

bool ConstMaterialParameter::isEqual(Vector3f p)
{
	HalleyAssertDev(type == ShaderParameterType::Float3);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Float3, &p);
}

bool ConstMaterialParameter::isEqual(Vector4f p)
{
	HalleyAssertDev(type == ShaderParameterType::Float4);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Float4, &p);
}

bool ConstMaterialParameter::isEqual(int p)
{
	HalleyAssertDev(type == ShaderParameterType::Int);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Int, &p);
}

bool ConstMaterialParameter::isEqual(Vector2i p)
{
	HalleyAssertDev(type == ShaderParameterType::Int2);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Int2, &p);
}

bool ConstMaterialParameter::isEqual(Vector3i p)
{
	HalleyAssertDev(type == ShaderParameterType::Int3);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Int3, &p);	
}

bool ConstMaterialParameter::isEqual(Vector4i p)
{
	HalleyAssertDev(type == ShaderParameterType::Int4);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Int4, &p);	
}

bool ConstMaterialParameter::isEqual(uint32_t p)
{
	HalleyAssertDev(type == ShaderParameterType::UInt);
	return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::UInt, &p);
}

bool ConstMaterialParameter::isEqual(const Matrix4f& m)
{
	HalleyAssertDev(type == ShaderParameterType::Matrix4);
	if (material->getDefinition().isColumnMajor()) {
		auto transposed = m;
		transposed.transpose();
		return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Matrix4, &transposed);
	} else {
		return material->isUniformEqualTo(blockNumber, offset, ShaderParameterType::Matrix4, &m);
	}
}