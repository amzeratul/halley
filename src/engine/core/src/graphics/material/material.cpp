#include <halley/support/exception.h>
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/shader.h"
#include "halley/api/video_api.h"
#include "halley/graphics/sprite/sprite_sheet.h"
#include "halley/utils/algorithm.h"
#include "halley/utils/hash.h"

using namespace Halley;

namespace {
	const Material* currentMaterial = nullptr;
	int currentPass = 0;
	uint64_t currentHash = 0;
}

MaterialDataBlock::MaterialDataBlock()
{
}

MaterialDataBlock::MaterialDataBlock(MaterialDataBlockType type, size_t size, int bindPoint, std::string_view name, const MaterialDefinition& def)
	: data(type == MaterialDataBlockType::SharedExternal ? 0 : size, 0)
	, dataBlockType(type)
	, bindPoint(static_cast<int16_t>(bindPoint))
{
}

MaterialDataBlock::MaterialDataBlock(MaterialDataBlock&& other) noexcept
	: data(std::move(other.data))
	, dataBlockType(other.dataBlockType)
	, needToUpdateHash(other.needToUpdateHash)
	, bindPoint(other.bindPoint)
	, hash(other.hash)
{
	other.hash = 0;
}

gsl::span<const gsl::byte> MaterialDataBlock::getData() const
{
	return gsl::as_bytes(gsl::span<const Byte>(data));
}

uint64_t MaterialDataBlock::getHash() const
{
	if (needToUpdateHash) {
		needToUpdateHash = false;
		Hash::Hasher hasher;
		hasher.feedBytes(getData());
		hash = hasher.digest();
	}
	return hash;
}

bool MaterialDataBlock::operator==(const MaterialDataBlock& other) const
{
	return getData() == other.getData();
}

bool MaterialDataBlock::operator!=(const MaterialDataBlock& other) const
{
	return getData() != other.getData();
}

bool MaterialDataBlock::setUniform(size_t offset, ShaderParameterType type, const void* srcData)
{
	Expects(dataBlockType != MaterialDataBlockType::SharedExternal);

	const size_t size = MaterialAttribute::getAttributeSize(type);
	Expects(size + offset <= data.size());
	Expects(offset % 4 == 0); // Alignment

	if (memcmp(data.data() + offset, srcData, size) != 0) {
		memcpy(data.data() + offset, srcData, size);
		needToUpdateHash = true;
		return true;
	} else {
		return false;
	}
}

bool MaterialDataBlock::isEqualTo(size_t offset, ShaderParameterType type, const void* srcData) const
{
	Expects(dataBlockType != MaterialDataBlockType::SharedExternal);

	const size_t size = MaterialAttribute::getAttributeSize(type);
	Expects(size + offset <= data.size());
	Expects(offset % 4 == 0); // Alignment

	return memcmp(data.data() + offset, srcData, size) == 0;
}

Material::Material(const Material& other)
	: materialDefinition(other.materialDefinition)
	, dataBlocks(other.dataBlocks)
	, textures(other.textures)
	, depthStencilEnabled(other.depthStencilEnabled)
	, stencilReferenceOverride(other.stencilReferenceOverride)
	, passEnabled(other.passEnabled)
{
}

Material::Material(Material&& other) noexcept
	: materialDefinition(std::move(other.materialDefinition))
	, dataBlocks(std::move(other.dataBlocks))
	, textures(std::move(other.textures))
	, depthStencilEnabled(other.depthStencilEnabled)
	, stencilReferenceOverride(other.stencilReferenceOverride)
	, passEnabled(other.passEnabled)
{
	other.fullHashValue = 0;
	other.partialHashValue = 0;
}

Material::Material(std::shared_ptr<const MaterialDefinition> definition, bool forceLocalBlocks)
	: materialDefinition(std::move(definition))
	, forceLocalBlocks(forceLocalBlocks)
{
	loadMaterialDefinition();
}

Material::~Material()
{
	textures.clear();
	dataBlocks.clear();
	materialDefinition = {};
}

void Material::loadMaterialDefinition()
{
	if (!materialDefinition) {
		return;
	}

	materialDefinition->waitForLoad();
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

	needToUpdateHash = true;
}

void Material::initUniforms(bool forceLocalBlocks)
{
	int nextBindPoint = 1;
	dataBlocks.clear();
	for (auto& uniformBlock : materialDefinition->getUniformBlocks()) {
		const auto type = uniformBlock.name == "HalleyBlock"
			? (forceLocalBlocks ? MaterialDataBlockType::SharedLocal : MaterialDataBlockType::SharedExternal)
			: MaterialDataBlockType::Local;
		const int bind = type == MaterialDataBlockType::Local ? nextBindPoint++ : 0;
		dataBlocks.push_back(MaterialDataBlock(type, uniformBlock.offset, bind, uniformBlock.name, *materialDefinition));
	}

	// Load textures
	const auto& textureDefs = materialDefinition->getTextures();
	const size_t nTextures = textureDefs.size();
	textures.resize(nTextures);
	for (size_t i = 0; i < nTextures; ++i) {
		textures[i] = textureDefs[i].defaultTexture;
	}
}

void Material::bind(int passNumber, Painter& painter) const
{
	// Avoid redundant work
	if (currentMaterial == this && currentPass == passNumber && currentHash == getFullHash()) {
		return;
	}
	currentMaterial = this;
	currentPass = passNumber;
	currentHash = getFullHash();

	painter.setMaterialPass(*this, passNumber);
}

void Material::resetBindCache()
{
	currentMaterial = nullptr;
	currentPass = 0;
	currentHash = 0;
}

bool Material::operator==(const Material& other) const
{
	// Same instance
	if (this == &other) {
		return true;
	}

	constexpr bool useHash = true;

	if (useHash) {
		// If the hashes match, we'll assume they're the same
		// There's a chance this will fail, but... what are the odds? :D
		// :D :D
		// :D :D :D
		return getFullHash() == other.getFullHash();
	} else {
		// Different definitions
		if (materialDefinition != other.materialDefinition) {
			return false;
		}
		
		// Different textures (only need to check pointer equality)
		if (textures != other.textures) {
			return false;
		}

		// Different data
		if (dataBlocks != other.dataBlocks) {
			return false;
		}

		// Different passes enabled
		if (passEnabled != other.passEnabled) {
			return false;
		}

		// Must be the same
		return true;
	}
}

bool Material::operator!=(const Material& material) const
{
	return !(*this == material);
}

bool Material::isCompatibleWith(const Material& other) const
{
	if (materialDefinition != other.materialDefinition) {
		return false;
	}

	if (textures != other.textures) {
		return false;
	}

	return true;
}

const Vector<std::shared_ptr<const Texture>>& Material::getTextures() const
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
		needToUpdateHash = true;
		return true;
	}
	return false;
}

bool Material::isUniformEqualTo(int blockNumber, size_t offset, ShaderParameterType type, const void* data) const
{
	return dataBlocks[blockNumber].isEqualTo(offset, type, data);
}

void Material::computeHashes() const
{
	Hash::Hasher hasher;

	hasher.feed(materialDefinition.get());
	
	for (const auto& dataBlock: dataBlocks) {
		hasher.feedBytes(dataBlock.getData());
	}

	hasher.feed(stencilReferenceOverride.has_value());
	hasher.feed(stencilReferenceOverride.value_or(0));
	hasher.feed(depthStencilEnabled);
	hasher.feed(passEnabled.to_ulong());

	partialHashValue = hasher.digest();

	for (const auto& texture: textures) {
		hasher.feed(texture.get());
	}

	fullHashValue = hasher.digest();
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

const std::shared_ptr<const Texture>& Material::getTexture(std::string_view name) const
{
	const auto& texs = materialDefinition->getTextures();
	for (size_t i = 0; i < texs.size(); ++i) {
		if (texs[i].name == name) {
			return getTexture(static_cast<int>(i));
		}
	}
	return getFallbackTexture();
}

std::shared_ptr<const Texture> Material::getRawTexture(int textureUnit) const
{
	return textureUnit >= 0 && textureUnit < static_cast<int>(textures.size()) ? textures[textureUnit] : std::shared_ptr<const Texture>();
}

const Vector<MaterialDataBlock>& Material::getDataBlocks() const
{
	return dataBlocks;
}

Vector<MaterialDataBlock>& Material::getDataBlocks()
{
	return dataBlocks;
}

void Material::setPassEnabled(int pass, bool enabled)
{
	if (passEnabled[pass] != enabled) {
		needToUpdateHash = true;
		passEnabled[pass] = enabled;
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

MaterialDepthStencil Material::getDepthStencil(int pass) const
{
	if (!depthStencilEnabled) {
		return {};
	}

	auto depthStencil = getDefinition().getPass(pass).getDepthStencil();
	if (stencilReferenceOverride) {
		depthStencil.setStencilReference(stencilReferenceOverride.value());
	}
	return depthStencil;
}

void Material::setStencilReferenceOverride(std::optional<uint8_t> reference)
{
	if (stencilReferenceOverride != reference) {
		stencilReferenceOverride = reference;
		needToUpdateHash = true;
	}
}

std::optional<uint8_t> Material::getStencilReferenceOverride() const
{
	return stencilReferenceOverride;
}

void Material::setDepthStencilEnabled(bool enabled)
{
	if (depthStencilEnabled != enabled) {
		depthStencilEnabled = enabled;
		needToUpdateHash = true;
	}
}

bool Material::isDepthStencilEnabled() const
{
	return depthStencilEnabled;
}

void Material::doSet(size_t textureUnit, const std::shared_ptr<const Texture>& texture)
{
	const auto& texs = materialDefinition->getTextures();
	if (textureUnit < texs.size()) {
		if (textures[textureUnit] != texture) {
			textures[textureUnit] = texture;
			needToUpdateHash = true;
		}
		return;
	}

	throw Exception("Texture unit \"" + toString(textureUnit) + "\" not available in material \"" + materialDefinition->getName() + "\"", HalleyExceptions::Graphics);
}

size_t Material::doSet(std::string_view name, const std::shared_ptr<const Texture>& texture)
{
	const auto& texs = materialDefinition->getTextures();
	for (size_t i = 0; i < texs.size(); ++i) {
		if (texs[i].name == name) {
			const auto textureUnit = i;
			if (textures[textureUnit] != texture) {
				textures[textureUnit] = texture;
				needToUpdateHash = true;
			}
			return i;
		}
	}

	throw Exception("Texture sampler \"" + String(name) + "\" not available in material \"" + materialDefinition->getName() + "\"", HalleyExceptions::Graphics);
}

Material& Material::set(std::string_view name, const std::shared_ptr<const Texture>& texture)
{
	doSet(name, texture);
	return *this;
}

Material& Material::set(std::string_view name, const std::shared_ptr<Texture>& texture)
{
	doSet(name, std::shared_ptr<const Texture>(texture));
	return *this;
}

Material& Material::set(std::string_view name, const SpriteResource& spriteResource)
{
	doSet(name, std::shared_ptr<const Texture>(spriteResource.getSpriteSheet()->getTexture()));
	return *this;
}

Material& Material::set(size_t textureUnit, const std::shared_ptr<const Texture>& texture)
{
	doSet(textureUnit, texture);
	return *this;
}

Material& Material::set(size_t textureUnit, const std::shared_ptr<Texture>& texture)
{
	doSet(textureUnit, std::shared_ptr<const Texture>(texture));
	return *this;
}

Material& Material::set(size_t textureUnit, const SpriteResource& spriteResource)
{
	doSet(textureUnit, spriteResource.getSpriteSheet()->getTexture());
	return *this;
}

const String& Material::getTexUnitAssetId(int texUnit) const
{
	if (texUnit < static_cast<int>(textures.size())) {
		if (textures[texUnit]) {
			return textures[texUnit]->getAssetId();
		}
	}
	return String::emptyString();
}

bool Material::hasParameter(std::string_view name) const
{
	for (const auto& block: materialDefinition->getUniformBlocks()) {
		for (const auto& u: block.uniforms) {
			if (u.name == name) {
				return true;
			}
		}
	}
	return false;
}

uint64_t Material::getPartialHash() const
{
	if (needToUpdateHash) {
		computeHashes();
		needToUpdateHash = false;
	}
	return partialHashValue;
}

uint64_t Material::getFullHash() const
{
	if (needToUpdateHash) {
		computeHashes();
		needToUpdateHash = false;
	}
	return fullHashValue;
}

MaterialParameter Material::getParameter(std::string_view name)
{
	for (const auto& block: materialDefinition->getUniformBlocks()) {
		for (const auto& u: block.uniforms) {
			if (u.name == name) {
				return MaterialParameter(*this, u.type, u.blockNumber, u.offset);
			}
		}
	}

	throw Exception("Uniform \"" + String(name) + "\" not available in material \"" + materialDefinition->getName() + "\"", HalleyExceptions::Graphics);
}

ConstMaterialParameter Material::getParameter(std::string_view name) const
{
	for (const auto& block: materialDefinition->getUniformBlocks()) {
		for (const auto& u: block.uniforms) {
			if (u.name == name) {
				return ConstMaterialParameter(*this, u.type, u.blockNumber, u.offset);
			}
		}
	}

	throw Exception("Uniform \"" + String(name) + "\" not available in material \"" + materialDefinition->getName() + "\"", HalleyExceptions::Graphics);
	
}

void Material::setDefinition(std::shared_ptr<const MaterialDefinition> definition)
{
	materialDefinition = std::move(definition);
	loadMaterialDefinition();
}

void Material::replaceMaterialDefinition(std::shared_ptr<const MaterialDefinition> definition)
{
	const auto oldMaterial = materialDefinition;
	const auto oldTextures = textures;
	const auto oldDataBlocks = dataBlocks;

	materialDefinition = std::move(definition);
	loadMaterialDefinition();

	const auto srcTexs = oldMaterial->getTextureNames();
	const auto dstTexs = materialDefinition->getTextureNames();

	// Remap textures
	for (size_t i = 0; i < oldTextures.size(); ++i) {
		if (oldTextures[i]) {
			const auto iter = std_ex::find(dstTexs, srcTexs[i]);
			if (iter != dstTexs.end()) {
				const int idx = static_cast<int>(iter - dstTexs.begin());
				set(idx, oldTextures[i]);
			}
		}
	}

	// Remap data blocks
	for (auto& dataBlock: dataBlocks) {
		for (auto& oldDataBlock: oldDataBlocks) {
			if (oldDataBlock.bindPoint == dataBlock.bindPoint && oldDataBlock.getType() == dataBlock.getType() && oldDataBlock.data.size() == dataBlock.data.size()) {
				dataBlock = oldDataBlock;
				break;
			}
		}
	}
}

std::shared_ptr<Material> Material::clone() const
{
	return std::make_shared<Material>(*this);
}


MaterialUpdater::MaterialUpdater(std::shared_ptr<const Material>& orig)
	: orig(&orig)
{
}

MaterialUpdater::MaterialUpdater(MaterialUpdater&& other) noexcept
{
	orig = other.orig;
	other.orig = nullptr;
	material = std::move(other.material);
}

MaterialUpdater::~MaterialUpdater()
{
	if (orig && material) {
		*orig = material;
	}
}

MaterialUpdater& MaterialUpdater::operator=(MaterialUpdater&& other) noexcept
{
	if (this != &other) {
		orig = other.orig;
		other.orig = nullptr;
		material = std::move(other.material);
	}
	return *this;
}

bool MaterialUpdater::isValid() const
{
	return orig != nullptr;
}

MaterialUpdater& MaterialUpdater::set(std::string_view name, const std::shared_ptr<const Texture>& texture)
{
	if (material || getOriginalMaterial().getTexture(name) != texture) {
		getWriteMaterial().set(name, texture);
	}
	return *this;
}

MaterialUpdater& MaterialUpdater::set(std::string_view name, const std::shared_ptr<Texture>& texture)
{
	if (material || getOriginalMaterial().getTexture(name) != texture) {
		getWriteMaterial().set(name, texture);
	}
	return *this;
}

MaterialUpdater& MaterialUpdater::set(std::string_view name, const SpriteResource& sprite)
{
	if (material || getOriginalMaterial().getTexture(name) != sprite.getSpriteSheet()->getTexture()) {
		getWriteMaterial().set(name, sprite);
	}
	return *this;
}

MaterialUpdater& MaterialUpdater::set(size_t textureUnit, const std::shared_ptr<const Texture>& texture)
{
	if (material || getOriginalMaterial().getTexture(static_cast<int>(textureUnit)) != texture) {
		getWriteMaterial().set(textureUnit, texture);
	}
	return *this;
}

MaterialUpdater& MaterialUpdater::set(size_t textureUnit, const std::shared_ptr<Texture>& texture)
{
	if (material || getOriginalMaterial().getTexture(static_cast<int>(textureUnit)) != texture) {
		getWriteMaterial().set(textureUnit, texture);
	}
	return *this;
}

MaterialUpdater& MaterialUpdater::set(size_t textureUnit, const SpriteResource& sprite)
{
	if (material || getOriginalMaterial().getTexture(static_cast<int>(textureUnit)) != sprite.getSpriteSheet()->getTexture()) {
		getWriteMaterial().set(textureUnit, sprite);
	}
	return *this;
}

MaterialUpdater& MaterialUpdater::setPassEnabled(int pass, bool enabled)
{
	if (material || getOriginalMaterial().isPassEnabled(pass) != enabled) {
		getWriteMaterial().setPassEnabled(pass, enabled);
	}
	return *this;
}

MaterialUpdater& MaterialUpdater::setStencilReferenceOverride(std::optional<uint8_t> reference)
{
	if (material || getOriginalMaterial().getStencilReferenceOverride() != reference) {
		getWriteMaterial().setStencilReferenceOverride(reference);
	}
	return *this;
}

MaterialUpdater& MaterialUpdater::setDepthStencilEnabled(bool enabled)
{
	if (material || getOriginalMaterial().isDepthStencilEnabled() != enabled) {
		getWriteMaterial().setDepthStencilEnabled(enabled);
	}
	return *this;
}

const std::shared_ptr<const Texture>& MaterialUpdater::getTexture(int textureUnit) const
{
	return getCurrentMaterial().getTexture(textureUnit);
}

std::shared_ptr<const Texture> MaterialUpdater::getRawTexture(int textureUnit) const
{
	return getCurrentMaterial().getRawTexture(textureUnit);
}

const Vector<std::shared_ptr<const Texture>>& MaterialUpdater::getTextures() const
{
	return getCurrentMaterial().getTextures();
}

size_t MaterialUpdater::getNumTextureUnits() const
{
	return getCurrentMaterial().getNumTextureUnits();
}

const MaterialDefinition& MaterialUpdater::getDefinition() const
{
	return getCurrentMaterial().getDefinition();
}

const std::shared_ptr<const MaterialDefinition>& MaterialUpdater::getDefinitionPtr() const
{
	return getCurrentMaterial().getDefinitionPtr();
}

MaterialUpdater& MaterialUpdater::setDefinition(std::shared_ptr<const MaterialDefinition> definition)
{
	if (material || getOriginalMaterial().getDefinitionPtr() != definition) {
		getWriteMaterial().setDefinition(std::move(definition));
	}
	return *this;
}

MaterialUpdater& MaterialUpdater::replaceMaterialDefinition(std::shared_ptr<const MaterialDefinition> definition)
{
	if (material || getOriginalMaterial().getDefinitionPtr() != definition) {
		getWriteMaterial().replaceMaterialDefinition(std::move(definition));
	}
	return *this;
}

const Material& MaterialUpdater::getCurrentMaterial() const
{
	return material ? *material : getOriginalMaterial();
}

const Material& MaterialUpdater::getOriginalMaterial() const
{
	return *(*orig);
}

Material& MaterialUpdater::getWriteMaterial()
{
	if (!material) {
		material = (*orig)->clone();
	}
	return *material;
}
