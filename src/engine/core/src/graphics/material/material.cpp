#include <halley/support/exception.h>
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/shader.h"
#include "halley/api/video_api.h"
#include "halley/graphics/sprite/sprite_sheet.h"
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

Material::Material(const Material& other)
	: materialDefinition(other.materialDefinition)
	, dataBlocks(other.dataBlocks)
	, textures(other.textures)
	, texUnitAssetId(other.texUnitAssetId)
	, stencilReferenceOverride(other.stencilReferenceOverride)
	, passEnabled(other.passEnabled)
{
}

Material::Material(Material&& other) noexcept
	: materialDefinition(std::move(other.materialDefinition))
	, dataBlocks(std::move(other.dataBlocks))
	, textures(std::move(other.textures))
	, texUnitAssetId(std::move(other.texUnitAssetId))
	, stencilReferenceOverride(other.stencilReferenceOverride)
	, passEnabled(other.passEnabled)
{
	other.fullHashValue = 0;
	other.partialHashValue = 0;
}

Material::Material(std::shared_ptr<const MaterialDefinition> definition, bool forceLocalBlocks)
	: materialDefinition(std::move(definition))
{
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
}

Material::~Material()
{
	textures.clear();
	dataBlocks.clear();
	materialDefinition = {};
}

void Material::initUniforms(bool forceLocalBlocks)
{
	int nextBindPoint = 1;
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
	textures.reserve(nTextures);
	for (size_t i = 0; i < nTextures; ++i) {
		const auto& tex = textureDefs[i];
		textures.push_back(tex.defaultTexture);
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

void Material::computeHashes() const
{
	Hash::Hasher hasher;

	hasher.feed(materialDefinition.get());
	
	for (const auto& dataBlock: dataBlocks) {
		hasher.feedBytes(dataBlock.getData());
	}

	hasher.feed(stencilReferenceOverride.has_value());
	hasher.feed(stencilReferenceOverride.value_or(0));
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

std::shared_ptr<const Texture> Material::getRawTexture(int textureUnit) const
{
	return textureUnit >= 0 && textureUnit < int(textures.size()) ? textures[textureUnit] : std::shared_ptr<const Texture>();
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
	if (stencilReferenceOverride) {
		auto depthStencil = getDefinition().getPass(pass).getDepthStencil();
		depthStencil.setStencilReference(stencilReferenceOverride.value());
		return depthStencil;
	} else {
		return getDefinition().getPass(pass).getDepthStencil();
	}
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
	const auto textureUnit = doSet(name, texture);
	setTexUnitAssetId(textureUnit, "");
	return *this;
}

Material& Material::set(std::string_view name, const std::shared_ptr<Texture>& texture)
{
	const auto textureUnit = doSet(name, std::shared_ptr<const Texture>(texture));
	setTexUnitAssetId(textureUnit, "");
	return *this;
}

Material& Material::set(std::string_view name, const SpriteResource& spriteResource)
{
	const auto textureUnit = doSet(name, std::shared_ptr<const Texture>(spriteResource.getSpriteSheet()->getTexture()));
	setTexUnitAssetId(textureUnit, spriteResource.getAssetId());
	return *this;
}

Material& Material::set(size_t textureUnit, const std::shared_ptr<const Texture>& texture)
{
	doSet(textureUnit, texture);
	setTexUnitAssetId(textureUnit, "");
	return *this;
}

Material& Material::set(size_t textureUnit, const std::shared_ptr<Texture>& texture)
{
	doSet(textureUnit, std::shared_ptr<const Texture>(texture));
	setTexUnitAssetId(textureUnit, "");
	return *this;
}

Material& Material::set(size_t textureUnit, const SpriteResource& spriteResource)
{
	doSet(textureUnit, spriteResource.getSpriteSheet()->getTexture());
	setTexUnitAssetId(textureUnit, spriteResource.getAssetId());
	return *this;
}

void Material::setTexUnitAssetId(size_t texUnit, const String& id)
{
	if (texUnitAssetId.size() != textures.size()) {
		texUnitAssetId.resize(textures.size());
	}
	texUnitAssetId[texUnit] = id;
}

const String& Material::getTexUnitAssetId(int texUnit) const
{
	if (false && texUnit < static_cast<int>(texUnitAssetId.size())) {
		if (!texUnitAssetId[texUnit].isEmpty()) {
			return texUnitAssetId[texUnit];
		}
	}
	if (texUnit < static_cast<int>(textures.size())) {
		return textures[texUnit]->getAssetId();
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

void Material::setDefinition(std::shared_ptr<const MaterialDefinition> definition)
{
	materialDefinition = std::move(definition);
}

std::shared_ptr<Material> Material::clone() const
{
	return std::make_shared<Material>(*this);
}


MaterialUpdater::MaterialUpdater(std::shared_ptr<const Material>& orig)
	: orig(&orig)
	, material(orig->clone())
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
	if (orig) {
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
	material->set(name, texture);
	return *this;
}

MaterialUpdater& MaterialUpdater::set(std::string_view name, const std::shared_ptr<Texture>& texture)
{
	material->set(name, texture);
	return *this;
}

MaterialUpdater& MaterialUpdater::set(std::string_view name, const SpriteResource& sprite)
{
	material->set(name, sprite);
	return *this;
}

MaterialUpdater& MaterialUpdater::set(size_t textureUnit, const std::shared_ptr<const Texture>& texture)
{
	material->set(textureUnit, texture);
	return *this;
}

MaterialUpdater& MaterialUpdater::set(size_t textureUnit, const std::shared_ptr<Texture>& texture)
{
	material->set(textureUnit, texture);
	return *this;
}

MaterialUpdater& MaterialUpdater::set(size_t textureUnit, const SpriteResource& sprite)
{
	material->set(textureUnit, sprite);
	return *this;
}

MaterialUpdater& MaterialUpdater::setPassEnabled(int pass, bool enabled)
{
	material->setPassEnabled(pass, enabled);
	return *this;
}

MaterialUpdater& MaterialUpdater::setStencilReferenceOverride(std::optional<uint8_t> reference)
{
	material->setStencilReferenceOverride(reference);
	return *this;
}

MaterialParameter MaterialUpdater::getParameter(std::string_view name)
{
	return material->getParameter(name);
}
