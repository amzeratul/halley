#include <halley/support/exception.h>
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/shader.h"
#include "api/video_api.h"
#include "halley/utils/hash.h"

using namespace Halley;

static Material* currentMaterial = nullptr;
static int currentPass = 0;

constexpr static int shaderStageCount = int(ShaderType::NumOfShaderTypes);

MaterialDataBlock::MaterialDataBlock()
{
}

MaterialDataBlock::MaterialDataBlock(MaterialDataBlockType type, size_t size, int bindPoint, const String& name, const MaterialDefinition& def)
	: data(type == MaterialDataBlockType::SharedExternal ? 0 : size, 0)
	, addresses(def.getNumPasses() * shaderStageCount)
	, dataBlockType(type)
	, bindPoint(bindPoint)
{
	for (int i = 0; i < def.getNumPasses(); ++i) {
		auto& shader = def.getPass(i).getShader();
		for (int j = 0; j < shaderStageCount; ++j) {
			addresses[i * shaderStageCount + j] = shader.getBlockLocation(name, ShaderType(j));
		}
	}
}

MaterialDataBlock::MaterialDataBlock(const MaterialDataBlock& other)
	: data(other.data)
	, addresses(other.addresses)
	, dataBlockType(other.dataBlockType)
	, bindPoint(other.bindPoint)
	, dirty(true)
{}

MaterialDataBlock::MaterialDataBlock(MaterialDataBlock&& other) noexcept
	: constantBuffer(std::move(other.constantBuffer))
	, data(std::move(other.data))
	, addresses(std::move(other.addresses))
	, dataBlockType(other.dataBlockType)
	, bindPoint(other.bindPoint)
	, dirty(other.dirty)
{}

MaterialConstantBuffer& MaterialDataBlock::getConstantBuffer() const
{
	Expects(constantBuffer != nullptr);
	return *constantBuffer;
}

int MaterialDataBlock::getAddress(int pass, ShaderType stage) const
{
	return addresses[pass * shaderStageCount + int(stage)];
}

int MaterialDataBlock::getBindPoint() const
{
	return bindPoint;
}

gsl::span<const gsl::byte> MaterialDataBlock::getData() const
{
	return gsl::as_bytes(gsl::span<const Byte>(data));
}

MaterialDataBlockType MaterialDataBlock::getType() const
{
	return dataBlockType;
}

bool MaterialDataBlock::setUniform(size_t offset, ShaderParameterType type, const void* srcData)
{
	Expects(dataBlockType != MaterialDataBlockType::SharedExternal);

	const size_t size = MaterialAttribute::getAttributeSize(type);
	Expects(size + offset <= data.size());
	Expects(offset % 4 == 0); // Alignment

	if (memcmp(data.data() + offset, srcData, size) != 0) {
		memcpy(data.data() + offset, srcData, size);
		dirty = true;
		return true;
	} else {
		return false;
	}
}

void MaterialDataBlock::upload(VideoAPI* api)
{
	if (dataBlockType != MaterialDataBlockType::SharedExternal) {
		if (!constantBuffer) {
			constantBuffer = api->createConstantBuffer();
			dirty = true;
		}
		if (dirty) {
			constantBuffer->update(*this);
			dirty = false;
		}
	}
}

Material::Material(const Material& other)
	: materialDefinition(other.materialDefinition)
	, uniforms(other.uniforms)
	, textureUniforms(other.textureUniforms)
	, dataBlocks(other.dataBlocks)
	, textures(other.textures)
	, passEnabled(other.passEnabled)
{
	for (auto& u: uniforms) {
		u.rebind(*this);
	}
}

Material::Material(Material&& other) noexcept
	: materialDefinition(std::move(other.materialDefinition))
	, uniforms(std::move(other.uniforms))
	, textureUniforms(std::move(other.textureUniforms))
	, dataBlocks(std::move(other.dataBlocks))
	, textures(std::move(other.textures))
	, passEnabled(other.passEnabled)
{
	for (auto& u: uniforms) {
		u.rebind(*this);
	}
}

Material::Material(std::shared_ptr<const MaterialDefinition> definition, bool forceLocalBlocks)
	: materialDefinition(std::move(definition))
{
	const size_t numPasses = materialDefinition->getNumPasses();
	if (numPasses > passEnabled.size()) {
		throw Exception("Too many passes in material.", HalleyExceptions::Graphics);
	}

	for (size_t i = 0; i < passEnabled.size(); ++i) {
		if (i < numPasses) {
			passEnabled[i] = materialDefinition->getPass(static_cast<int>(i)).isEnabled();
		} else {
			passEnabled[i] = false;
		}
	}
	
	initUniforms(forceLocalBlocks);
}

void Material::initUniforms(bool forceLocalBlocks)
{
	int blockNumber = 0;
	int nextBindPoint = 1;
	for (auto& uniformBlock : materialDefinition->getUniformBlocks()) {
		size_t curOffset = 0;
		for (auto& uniform: uniformBlock.uniforms) {
			auto size = MaterialAttribute::getAttributeSize(uniform.type);
			curOffset = alignUp(curOffset, std::min(size_t(16), size));
			uniforms.push_back(MaterialParameter(*this, uniform.name, uniform.type, blockNumber, curOffset));
			curOffset += size;
		}
		auto type = uniformBlock.name == "HalleyBlock"
			? (forceLocalBlocks ? MaterialDataBlockType::SharedLocal : MaterialDataBlockType::SharedExternal)
			: MaterialDataBlockType::Local;
		int bind = type == MaterialDataBlockType::Local ? nextBindPoint++ : 0;
		dataBlocks.push_back(MaterialDataBlock(type, curOffset, bind, uniformBlock.name, *materialDefinition));
		++blockNumber;
	}

	// Load textures
	const auto& textureDefs = materialDefinition->getTextures();
	const size_t nTextures = textureDefs.size();
	textures.reserve(nTextures);
	textureUniforms.reserve(nTextures);
	for (size_t i = 0; i < nTextures; ++i) {
		const auto& tex = textureDefs[i];
		textureUniforms.push_back(MaterialTextureParameter(*this, tex.name));
		textures.push_back(tex.defaultTexture);
	}
}

void Material::bind(int passNumber, Painter& painter)
{
	// Avoid redundant work
	if (currentMaterial == this && currentPass == passNumber && !needToUploadData) {
		return;
	}
	currentMaterial = this;
	currentPass = passNumber;

	painter.setMaterialPass(*this, passNumber);
}

void Material::uploadData(Painter& painter)
{	
	if (needToUploadData) {
		for (auto& block: dataBlocks) {
			block.upload(getDefinition().api);
		}
		needToUploadData = false;
	}
}

void Material::resetBindCache()
{
	currentMaterial = nullptr;
	currentPass = 0;
}

bool Material::operator==(const Material& other) const
{
	// Same instance
	if (this == &other) {
		return true;
	}

	// Different definitions
	if (materialDefinition != other.materialDefinition) {
		return false;
	}

	constexpr bool useHash = true;

	if (useHash) {
		// If the hashes match, we'll assume they're the same
		// There's a chance this will fail, but... what are the odds? :D
		// :D :D
		// :D :D :D
		return getHash() == other.getHash();
	} else {
		// Different textures (only need to check pointer equality)
		for (size_t i = 0; i < textures.size(); ++i) {
			if (textures[i] != other.textures[i]) {
				return false;
			}
		}

		// Different data
		for (size_t i = 0; i < dataBlocks.size(); ++i) {
			if (dataBlocks[i].getData() != other.dataBlocks[i].getData()) {
				return false;
			}
		}

		// Different passes enabled
		for (size_t i = 0; i < passEnabled.size(); ++i) {
			if (passEnabled[i] != other.passEnabled[i]) {
				return false;
			}
		}

		// Must be the same
		return true;
	}
}

bool Material::operator!=(const Material& material) const
{
	return !(*this == material);
}

const std::vector<std::shared_ptr<const Texture>>& Material::getTextures() const
{
	return textures;
}

size_t Material::getNumTextureUnits() const
{
	return textures.size();
}

bool Material::setUniform(int blockNumber, size_t offset, ShaderParameterType type, const void* data)
{
	if (dataBlocks[blockNumber].setUniform(offset, type, data)) {
		needToUploadData = true;
		needToUpdateHash = true;
		return true;
	}
	return false;
}

uint64_t Material::computeHash() const
{
	Hash::Hasher hasher;
	
	for (const auto& texture: textures) {
		hasher.feed(texture.get());
	}

	for (const auto& dataBlock: dataBlocks) {
		hasher.feedBytes(dataBlock.getData());
	}

	hasher.feedBytes(gsl::as_bytes(gsl::span<const bool>(passEnabled.data(), passEnabled.size())));

	return hasher.digest();
}

const std::shared_ptr<const Texture>& Material::getFallbackTexture() const
{
	return materialDefinition->getFallbackTexture();
}

const std::shared_ptr<const Texture>& Material::getTexture(int textureUnit) const
{
	auto& tex = textureUnit >= 0 && textureUnit < int(textures.size()) ? textures[textureUnit] : getFallbackTexture();
	if (!tex) {
		return getFallbackTexture();
	}
	return tex;
}

const Vector<MaterialParameter>& Material::getUniforms() const
{
	return uniforms;
}

const Vector<MaterialDataBlock>& Material::getDataBlocks() const
{
	return dataBlocks;
}

void Material::setPassEnabled(int pass, bool enabled)
{
	auto& p = passEnabled.at(pass);
	if (p != enabled) {
		needToUpdateHash = true;
		p = enabled;
	}
}

bool Material::isPassEnabled(int pass) const
{
	const size_t p = static_cast<size_t>(pass);
	if (p > passEnabled.size()) {
		return false;
	}
	return passEnabled[p];
}

const Vector<MaterialTextureParameter>& Material::getTextureUniforms() const
{
	return textureUniforms;
}

Material& Material::set(const String& name, const std::shared_ptr<const Texture>& texture)
{
	auto& texs = materialDefinition->getTextures();
	for (size_t i = 0; i < texs.size(); ++i) {
		if (texs[i].name == name) {
			const auto textureUnit = i;
			if (textures[textureUnit] != texture) {
				textures[textureUnit] = texture;
				needToUpdateHash = true;
			}
			return *this;
		}
	}

	throw Exception("Texture sampler \"" + name + "\" not available in material \"" + materialDefinition->getName() + "\"", HalleyExceptions::Graphics);
}

Material& Material::set(const String& name, const std::shared_ptr<Texture>& texture)
{
	return set(name, std::shared_ptr<const Texture>(texture));
}

bool Material::hasParameter(const String& name) const
{
	for (auto& u: uniforms) {
		if (u.name == name) {
			return true;
		}
	}
	return false;
}

uint64_t Material::getHash() const
{
	if (needToUpdateHash) {
		hashValue = computeHash();
		needToUpdateHash = false;
	}
	return hashValue;
}

MaterialParameter& Material::getParameter(const String& name)
{
	for (auto& u : uniforms) {
		if (u.name == name) {
			return u;
		}
	}

	throw Exception("Uniform \"" + name + "\" not available in material \"" + materialDefinition->getName() + "\"", HalleyExceptions::Graphics);
}

std::shared_ptr<Material> Material::clone() const
{
	return std::make_shared<Material>(*this);
}
