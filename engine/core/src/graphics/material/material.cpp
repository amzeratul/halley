#include <halley/support/exception.h>
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/painter.h"
#include "api/video_api.h"
#include <cstring>

using namespace Halley;

static Material* currentMaterial = nullptr;
static int currentPass = 0;

Material::Material(const Material& other)
	: uniforms(other.uniforms)
	, materialDefinition(other.materialDefinition)
	, uniformData(other.uniformData)
	, textures(other.textures)
{	
}

Material::Material(std::shared_ptr<const MaterialDefinition> materialDefinition)
	: materialDefinition(materialDefinition)
{
	initUniforms();
}

void Material::initUniforms()
{
	size_t curOffset = 0;
	int textureUnit = 0;
	for (auto& uniform : materialDefinition->getUniforms()) {
		uniforms.push_back(MaterialParameter(*this, uniform.name, uniform.type, curOffset));
		curOffset += MaterialAttribute::getAttributeSize(uniform.type);

		auto& u = uniforms.back();
		u.init();
		if (uniform.type == ShaderParameterType::Texture2D) {
			u.textureUnit = textureUnit++;
		}
	}
	uniformData.resize(curOffset);
	textures.resize(std::max(1, textureUnit));
}

void Material::bind(int passNumber, Painter& painter)
{
	// Avoid redundant work
	if (currentMaterial == this && currentPass == passNumber && !dirty) {
		return;
	}
	currentMaterial = this;
	currentPass = passNumber;

	if (!constantBuffer) {
		constantBuffer = getDefinition().api->createConstantBuffer(*this);
		dirty = true;
	}
	if (dirty) {
		constantBuffer->update(*this);
		dirty = false;
	}
	painter.setMaterialPass(*this, passNumber);
}

void Material::resetBindCache()
{
	currentMaterial = nullptr;
	currentPass = 0;
}

void Material::setUniform(size_t offset, ShaderParameterType type, void* data)
{
	const size_t size = MaterialAttribute::getAttributeSize(type);
	Expects(size + offset <= uniformData.size());
	Expects(offset % 4 == 0); // Alignment
	memcpy(uniformData.data() + offset, data, size);
}

void Material::setTexture(int textureUnit, std::shared_ptr<const Texture> texture)
{
	textures[textureUnit] = texture;
}

const std::shared_ptr<const Texture>& Material::getMainTexture() const
{
	return textures[0];
}

const std::shared_ptr<const Texture>& Material::getTexture(int textureUnit) const
{
	return textures[textureUnit];
}

const Bytes& Material::getData() const
{
	return uniformData;
}

const Vector<MaterialParameter>& Material::getUniforms() const
{
	return uniforms;
}

MaterialConstantBuffer& Material::getConstantBuffer() const
{
	Expects(constantBuffer);
	return *constantBuffer;
}

MaterialParameter* Material::getParameter(const String& name)
{
	for (auto& u : uniforms) {
		if (u.name == name) {
			return &u;
		}
	}
	return nullptr;
}

std::shared_ptr<Material> Material::clone() const
{
	return std::make_shared<Material>(*this);
}
