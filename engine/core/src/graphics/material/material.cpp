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
	: materialDefinition(other.materialDefinition)
	, uniforms(other.uniforms)
	, uniformData(other.uniformData)
	, textureUniforms(other.textureUniforms)
	, textures(other.textures)
{
	for (auto& u: uniforms) {
		u.rebind(*this);
	}
}

Material::Material(std::shared_ptr<const MaterialDefinition> materialDefinition)
	: materialDefinition(materialDefinition)
{
	initUniforms();
}

void Material::initUniforms()
{
	size_t curOffset = 0;
	for (auto& uniform : materialDefinition->getUniforms()) {
		uniforms.push_back(MaterialParameter(*this, uniform.name, uniform.type, curOffset));
		curOffset += MaterialAttribute::getAttributeSize(uniform.type);
	}
	uniformData.resize(curOffset);

	textures.resize(std::max(size_t(1), materialDefinition->getTextures().size()));
	for (auto& tex: materialDefinition->getTextures()) {
		textureUniforms.push_back(MaterialParameter(*this, tex, ShaderParameterType::Texture2D, 0));
	}
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

const Vector<MaterialParameter>& Material::getTextureUniforms() const
{
	return textureUniforms;
}

MaterialConstantBuffer& Material::getConstantBuffer() const
{
	Expects(constantBuffer);
	return *constantBuffer;
}

Material& Material::set(const String& name, const std::shared_ptr<const Texture>& texture)
{
	auto& texs = materialDefinition->getTextures();
	for (size_t i = 0; i < texs.size(); ++i) {
		if (texs[i] == name) {
			auto textureUnit = i;
			textures[textureUnit] = texture;
			return *this;
		}
	}

	throw Exception("Texture sampler \"" + name + "\" not available in material \"" + materialDefinition->getName() + "\"");
}

MaterialParameter& Material::getParameter(const String& name)
{
	for (auto& u : uniforms) {
		if (u.name == name) {
			return u;
		}
	}

	throw Exception("Uniform \"" + name + "\" not available in material \"" + materialDefinition->getName() + "\"");
}

std::shared_ptr<Material> Material::clone() const
{
	return std::make_shared<Material>(*this);
}
