#pragma once

#include <memory>
#include "halley/text/halleystring.h"
#include "halley/graphics/texture.h"
#include "halley/graphics/material/material_parameter.h"
#include <gsl/gsl>
#include <bitset>

namespace Halley
{
	class SpriteResource;
	class MaterialDepthStencil;
	enum class ShaderType;
	class MaterialDataBlock;
	class Material;
	enum class ShaderParameterType : uint8_t;
	class Painter;
	class MaterialDefinition;
	class MaterialParameter;
	class VideoAPI;

	class MaterialConstantBuffer
	{
	public:
		virtual ~MaterialConstantBuffer() {}

		virtual void update(gsl::span<const gsl::byte> data) = 0;
	};

	enum class MaterialDataBlockType : uint8_t
	{
		// Shared blocks are not stored locally in the material (e.g. the HalleyBlock, stored by the engine)
		SharedLocal,    // Shared, this keeps the canonical copy
		SharedExternal, // Shared, only a reference
		Local           // Local data
	};

	class MaterialDataBlock
	{
		friend class Material;

	public:
		MaterialDataBlock();
		MaterialDataBlock(MaterialDataBlockType type, size_t size, int bindPoint, std::string_view name, const MaterialDefinition& def);
		MaterialDataBlock(const MaterialDataBlock& other) = default;
		MaterialDataBlock(MaterialDataBlock&& other) noexcept;

		int getBindPoint() const { return bindPoint; }
		gsl::span<const gsl::byte> getData() const;
		MaterialDataBlockType getType() const { return dataBlockType; }
		uint64_t getHash() const;

		MaterialDataBlock& operator=(const MaterialDataBlock& other) = default;
		MaterialDataBlock& operator=(MaterialDataBlock&& other) = default;

		bool operator==(const MaterialDataBlock& other) const;
		bool operator!=(const MaterialDataBlock& other) const;

	private:
		Bytes data;
		MaterialDataBlockType dataBlockType = MaterialDataBlockType::Local;
		mutable bool needToUpdateHash = true;
		int16_t bindPoint = 0;
		mutable uint64_t hash = 0;

		bool setUniform(size_t offset, ShaderParameterType type, const void* data);
		bool isEqualTo(size_t offset, ShaderParameterType type, const void* data) const;
	};
	
	class Material
	{
		friend class MaterialParameter;
		friend class ConstMaterialParameter;

	public:
		Material(const Material& other);
		Material(Material&& other) noexcept;
		explicit Material(std::shared_ptr<const MaterialDefinition> materialDefinition, bool forceLocalBlocks = false); // forceLocalBlocks is for engine use only
		~Material();

		void bind(int pass, Painter& painter) const;
		static void resetBindCache();

		bool operator==(const Material& material) const;
		bool operator!=(const Material& material) const;

		bool isCompatibleWith(const Material& other) const;

		const MaterialDefinition& getDefinition() const { return *materialDefinition; }
		const std::shared_ptr<const MaterialDefinition>& getDefinitionPtr() const { return materialDefinition; }
		void setDefinition(std::shared_ptr<const MaterialDefinition> definition);
		void replaceMaterialDefinition(std::shared_ptr<const MaterialDefinition> definition);

		std::shared_ptr<Material> clone() const;
		
		const std::shared_ptr<const Texture>& getTexture(int textureUnit) const;
		const std::shared_ptr<const Texture>& getTexture(std::string_view name) const;
		std::shared_ptr<const Texture> getRawTexture(int textureUnit) const;
		const Vector<std::shared_ptr<const Texture>>& getTextures() const;
		size_t getNumTextureUnits() const;

		const Vector<MaterialDataBlock>& getDataBlocks() const;
		Vector<MaterialDataBlock>& getDataBlocks();

		void setPassEnabled(int pass, bool enabled);
		bool isPassEnabled(int pass) const;
		const std::bitset<8>& getPassesEnabled() const { return passEnabled; }

		MaterialDepthStencil getDepthStencil(int pass) const;
		void setStencilReferenceOverride(std::optional<uint8_t> reference);
		std::optional<uint8_t> getStencilReferenceOverride() const;
		void setDepthStencilEnabled(bool enabled);
		bool isDepthStencilEnabled() const;

		Material& set(std::string_view name, const std::shared_ptr<const Texture>& texture);
		Material& set(std::string_view name, const std::shared_ptr<Texture>& texture);
		Material& set(std::string_view name, const SpriteResource& spriteResource);
		Material& set(size_t textureUnit, const std::shared_ptr<const Texture>& texture);
		Material& set(size_t textureUnit, const std::shared_ptr<Texture>& texture);
		Material& set(size_t textureUnit, const SpriteResource& spriteResource);
		
		MaterialParameter getParameter(std::string_view name);
		ConstMaterialParameter getParameter(std::string_view name) const;
		bool hasParameter(std::string_view name) const;
		
		template <typename T>
		Material& set(std::string_view name, const T& value)
		{
			assert(!name.empty());
			getParameter(name).set(value);
			return *this;
		}

		uint64_t getPartialHash() const; // Not including textures
		uint64_t getFullHash() const; // Including textures

		const String& getTexUnitAssetId(int texUnit) const;

	private:
		std::shared_ptr<const MaterialDefinition> materialDefinition;
		
		Vector<MaterialDataBlock> dataBlocks;
		Vector<std::shared_ptr<const Texture>> textures;

		mutable bool needToUpdateHash = true;
		bool forceLocalBlocks = false;
		bool depthStencilEnabled = true;
		std::optional<uint8_t> stencilReferenceOverride;
		std::bitset<8> passEnabled;
		mutable uint64_t fullHashValue = 0;
		mutable uint64_t partialHashValue = 0;

		void doSet(size_t textureUnit, const std::shared_ptr<const Texture>& texture);
		size_t doSet(std::string_view name, const std::shared_ptr<const Texture>& texture);

		void loadMaterialDefinition();
		void initUniforms(bool forceLocalBlocks);

		bool setUniform(int blockNumber, size_t offset, ShaderParameterType type, const void* data);
		bool isUniformEqualTo(int blockNumber, size_t offset, ShaderParameterType type, const void* data) const;
		void computeHashes() const;

		const std::shared_ptr<const Texture>& getFallbackTexture() const;
	};

	class MaterialUpdater {
	public:
		MaterialUpdater() = default;
		MaterialUpdater(std::shared_ptr<const Material>& orig);
		MaterialUpdater(const MaterialUpdater& other) = delete;
		MaterialUpdater(MaterialUpdater&& other) noexcept;
		~MaterialUpdater();

		MaterialUpdater& operator=(const MaterialUpdater& other) = delete;
		MaterialUpdater& operator=(MaterialUpdater&& other) noexcept;

		bool isValid() const;

		MaterialUpdater& set(std::string_view name, const std::shared_ptr<const Texture>& texture);
		MaterialUpdater& set(std::string_view name, const std::shared_ptr<Texture>& texture);
		MaterialUpdater& set(std::string_view name, const SpriteResource& sprite);
		MaterialUpdater& set(size_t textureUnit, const std::shared_ptr<const Texture>& texture);
		MaterialUpdater& set(size_t textureUnit, const std::shared_ptr<Texture>& texture);
		MaterialUpdater& set(size_t textureUnit, const SpriteResource& sprite);

		template <typename T>
		MaterialUpdater& set(std::string_view name, const T& value)
		{
			if (material || !getOriginalMaterial().getParameter(name).isEqual(value)) {
				getWriteMaterial().getParameter(name).set(value);
			}
			return *this;
		}

		MaterialUpdater& setPassEnabled(int pass, bool enabled);
		MaterialUpdater& setStencilReferenceOverride(std::optional<uint8_t> reference);
		MaterialUpdater& setDepthStencilEnabled(bool enabled);

		const std::shared_ptr<const Texture>& getTexture(int textureUnit) const;
		std::shared_ptr<const Texture> getRawTexture(int textureUnit) const;
		const Vector<std::shared_ptr<const Texture>>& getTextures() const;
		size_t getNumTextureUnits() const;

		const MaterialDefinition& getDefinition() const;
		const std::shared_ptr<const MaterialDefinition>& getDefinitionPtr() const;
		MaterialUpdater& setDefinition(std::shared_ptr<const MaterialDefinition> definition);
		MaterialUpdater& replaceMaterialDefinition(std::shared_ptr<const MaterialDefinition> definition);

	private:
		std::shared_ptr<const Material>* orig = nullptr;
		std::shared_ptr<Material> material;

		const Material& getCurrentMaterial() const;
		const Material& getOriginalMaterial() const;
		Material& getWriteMaterial();
	};
}
