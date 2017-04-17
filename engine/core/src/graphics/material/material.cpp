#include <halley/support/exception.h>
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/shader.h"
#include "api/video_api.h"

using namespace Halley;

static Material* currentMaterial = nullptr;
static int currentPass = 0;

MaterialDataBlock::MaterialDataBlock()
{
}

MaterialDataBlock::MaterialDataBlock(size_t size, const String& name, const MaterialDefinition& def)
	: data(size)
	, addresses(def.getNumPasses())
{
	for (int i = 0; i < def.getNumPasses(); ++i) {
		auto& shader = def.getPass(i).getShader();
		addresses[i] = shader.getBlockLocation(name);
	}
}

MaterialDataBlock::MaterialDataBlock(const MaterialDataBlock& other)
	: data(other.data)
	, addresses(other.addresses)
	, dirty(true)
{
}

MaterialDataBlock::MaterialDataBlock(MaterialDataBlock&& other) noexcept
	: constantBuffer(std::move(other.constantBuffer))
	, data(std::move(other.data))
	, addresses(std::move(other.addresses))
	, dirty(other.dirty)
{
}

MaterialConstantBuffer& MaterialDataBlock::getConstantBuffer() const
{
	Expects(constantBuffer);
	return *constantBuffer;
}

int MaterialDataBlock::getAddress(int pass) const
{
	return addresses[pass];
}

gsl::span<const gsl::byte> MaterialDataBlock::getData() const
{
	return gsl::as_bytes(gsl::span<const Byte>(data));
}

void MaterialDataBlock::setUniform(size_t offset, ShaderParameterType type, void* srcData)
{
	const size_t size = MaterialAttribute::getAttributeSize(type);
	Expects(size + offset <= data.size());
	Expects(offset % 4 == 0); // Alignment
	memcpy(data.data() + offset, srcData, size);
	dirty = true;
}

void MaterialDataBlock::upload(int passNumber, VideoAPI* api)
{
	if (!constantBuffer) {
		constantBuffer = api->createConstantBuffer();
		dirty = true;
	}
	if (dirty) {
		constantBuffer->update(*this);
		dirty = false;
	}
}

Material::Material(const Material& other)
	: materialDefinition(other.materialDefinition)
	, uniforms(other.uniforms)
	, textureUniforms(other.textureUniforms)
	, dataBlocks(other.dataBlocks)
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
	int blockNumber = 0;
	for (auto& uniformBlock : materialDefinition->getUniformBlocks()) {
		size_t curOffset = 0;
		for (auto& uniform: uniformBlock.uniforms) {
			auto size = MaterialAttribute::getAttributeSize(uniform.type);
			curOffset = alignUp(curOffset, std::min(size_t(16), size));
			uniforms.push_back(MaterialParameter(*this, uniform.name, uniform.type, blockNumber, curOffset));
			curOffset += size;
		}
		dataBlocks.push_back(MaterialDataBlock(curOffset, uniformBlock.name, *materialDefinition));
		++blockNumber;
	}

	textures.resize(std::max(size_t(1), materialDefinition->getTextures().size()));
	for (auto& tex: materialDefinition->getTextures()) {
		textureUniforms.push_back(MaterialTextureParameter(*this, tex));
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

	for (auto& block: dataBlocks) {
		block.upload(passNumber, getDefinition().api);
	}
	dirty = false;

	painter.setMaterialPass(*this, passNumber);
}

void Material::resetBindCache()
{
	currentMaterial = nullptr;
	currentPass = 0;
}

void Material::setUniform(int blockNumber, size_t offset, ShaderParameterType type, void* data)
{
	dataBlocks[blockNumber].setUniform(offset, type, data);
	dirty = true;
}

const std::shared_ptr<const Texture>& Material::getMainTexture() const
{
	return textures[0];
}

const std::shared_ptr<const Texture>& Material::getTexture(int textureUnit) const
{
	return textures[textureUnit];
}

const Vector<MaterialParameter>& Material::getUniforms() const
{
	return uniforms;
}

const Vector<MaterialDataBlock>& Material::getDataBlocks() const
{
	return dataBlocks;
}

const Vector<MaterialTextureParameter>& Material::getTextureUniforms() const
{
	return textureUniforms;
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

Material& Material::set(const String& name, const std::shared_ptr<Texture>& texture)
{
	return set(name, std::shared_ptr<const Texture>(texture));
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
