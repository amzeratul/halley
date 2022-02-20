#pragma once

#include <memory>
#include "halley/text/halleystring.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <gsl/gsl>
#include <bitset>

namespace Halley
{
	class MaterialDepthStencil;
	enum class ShaderType;
	class MaterialDataBlock;
	class Material;
	enum class ShaderParameterType : uint8_t;
	class Painter;
	class MaterialDefinition;
	class MaterialParameter;
	class MaterialTextureParameter;
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
		MaterialDataBlock(MaterialDataBlockType type, size_t size, int bindPoint, const String& name, const MaterialDefinition& def);
		MaterialDataBlock(const MaterialDataBlock& other);
		MaterialDataBlock(MaterialDataBlock&& other) noexcept;

		int getAddress(int pass, ShaderType stage) const;
		int getBindPoint() const;
		gsl::span<const gsl::byte> getData() const;
		MaterialDataBlockType getType() const;
		uint64_t getHash() const;

	private:
		Bytes data;
		Vector<int> addresses;
		MaterialDataBlockType dataBlockType = MaterialDataBlockType::Local;
		mutable bool needToUpdateHash = true;
		int16_t bindPoint = 0;
		mutable uint64_t hash = 0;

		bool setUniform(size_t offset, ShaderParameterType type, const void* data);
	};
	
	class Material
	{
		friend class MaterialParameter;

	public:
		Material(const Material& other);
		Material(Material&& other) noexcept;
		explicit Material(std::shared_ptr<const MaterialDefinition> materialDefinition, bool forceLocalBlocks = false); // forceLocalBlocks is for engine use only

		void bind(int pass, Painter& painter);
		static void resetBindCache();

		bool operator==(const Material& material) const;
		bool operator!=(const Material& material) const;

		bool isCompatibleWith(const Material& other) const;

		const MaterialDefinition& getDefinition() const { return *materialDefinition; }
		const std::shared_ptr<const MaterialDefinition>& getDefinitionPtr() const { return materialDefinition; }

		std::shared_ptr<Material> clone() const;
		
		const std::shared_ptr<const Texture>& getTexture(int textureUnit) const;
		std::shared_ptr<const Texture> getRawTexture(int textureUnit) const;
		const Vector<MaterialTextureParameter>& getTextureUniforms() const;
		const Vector<std::shared_ptr<const Texture>>& getTextures() const;
		size_t getNumTextureUnits() const;

		const Vector<MaterialParameter>& getUniforms() const;
		const Vector<MaterialDataBlock>& getDataBlocks() const;

		void setPassEnabled(int pass, bool enabled);
		bool isPassEnabled(int pass) const;

		MaterialDepthStencil getDepthStencil(int pass) const;
		void setStencilReferenceOverride(std::optional<uint8_t> reference);
		std::optional<uint8_t> getStencilReferenceOverride() const;

		Material& set(const String& name, const std::shared_ptr<const Texture>& texture);
		Material& set(const String& name, const std::shared_ptr<Texture>& texture);
		Material& set(size_t textureUnit, const std::shared_ptr<const Texture>& texture);
		Material& set(size_t textureUnit, const std::shared_ptr<Texture>& texture);
		
		bool hasParameter(const String& name) const;

		template <typename T>
		Material& set(const String& name, const T& value)
		{
			getParameter(name).set(value);
			return *this;
		}

		uint64_t getHash() const;

	private:
		std::shared_ptr<const MaterialDefinition> materialDefinition;
		
		Vector<MaterialParameter> uniforms;
		Vector<MaterialTextureParameter> textureUniforms;
		Vector<MaterialDataBlock> dataBlocks;
		Vector<std::shared_ptr<const Texture>> textures;

		mutable uint64_t hashValue = 0;
		mutable bool needToUpdateHash = true;
		std::optional<uint8_t> stencilReferenceOverride;
		std::bitset<8> passEnabled;

		void initUniforms(bool forceLocalBlocks);
		MaterialParameter& getParameter(const String& name);

		bool setUniform(int blockNumber, size_t offset, ShaderParameterType type, const void* data);
		uint64_t computeHash() const;

		const std::shared_ptr<const Texture>& getFallbackTexture() const;
	};
}
