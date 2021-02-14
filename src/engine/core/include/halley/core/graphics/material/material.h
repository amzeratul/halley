#pragma once

#include <memory>
#include "halley/text/halleystring.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <gsl/gsl>

namespace Halley
{
	enum class ShaderType;
	class MaterialDataBlock;
	class Material;
	enum class ShaderParameterType;
	class Painter;
	class MaterialDefinition;
	class MaterialParameter;
	class MaterialTextureParameter;
	class VideoAPI;

	class MaterialConstantBuffer
	{
	public:
		virtual ~MaterialConstantBuffer() {}

		virtual void update(const MaterialDataBlock& dataBlock) = 0;
	};

	enum class MaterialDataBlockType
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

		MaterialConstantBuffer& getConstantBuffer() const;
		int getAddress(int pass, ShaderType stage) const;
		int getBindPoint() const;
		gsl::span<const gsl::byte> getData() const;
		MaterialDataBlockType getType() const;

	private:
		std::unique_ptr<MaterialConstantBuffer> constantBuffer;
		Bytes data;
		Vector<int> addresses;
		MaterialDataBlockType dataBlockType;
		int bindPoint = 0;
		bool dirty = true;

		bool setUniform(size_t offset, ShaderParameterType type, const void* data);
		void upload(VideoAPI* api);
	};
	
	class Material
	{
		friend class MaterialParameter;

	public:
		Material(const Material& other);
		Material(Material&& other) noexcept;
		explicit Material(std::shared_ptr<const MaterialDefinition> materialDefinition, bool forceLocalBlocks = false); // forceLocalBlocks is for engine use only

		void bind(int pass, Painter& painter);
		void uploadData(Painter& painter);
		static void resetBindCache();

		bool operator==(const Material& material) const;
		bool operator!=(const Material& material) const;

		bool isCompatibleWith(const Material& other) const;

		const MaterialDefinition& getDefinition() const { return *materialDefinition; }

		std::shared_ptr<Material> clone() const;
		
		const std::shared_ptr<const Texture>& getTexture(int textureUnit) const;
		const Vector<MaterialTextureParameter>& getTextureUniforms() const;
		const std::vector<std::shared_ptr<const Texture>>& getTextures() const;
		size_t getNumTextureUnits() const;

		const Vector<MaterialParameter>& getUniforms() const;
		const Vector<MaterialDataBlock>& getDataBlocks() const;

		void setPassEnabled(int pass, bool enabled);
		bool isPassEnabled(int pass) const;

		Material& set(const String& name, const std::shared_ptr<const Texture>& texture);
		Material& set(const String& name, const std::shared_ptr<Texture>& texture);

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
		std::vector<std::shared_ptr<const Texture>> textures;
		std::array<bool, 8> passEnabled;

		mutable uint64_t hashValue = 0;
		mutable bool needToUpdateHash = true;
		bool needToUploadData = true;

		void initUniforms(bool forceLocalBlocks);
		MaterialParameter& getParameter(const String& name);

		bool setUniform(int blockNumber, size_t offset, ShaderParameterType type, const void* data);
		uint64_t computeHash() const;

		const std::shared_ptr<const Texture>& getFallbackTexture() const;
	};

	class MaterialHandle {
	public:
		MaterialHandle() = default;
		MaterialHandle(const MaterialHandle& other) = default;
		MaterialHandle(MaterialHandle&& other) noexcept = default;
		MaterialHandle(std::shared_ptr<Material> material);
		explicit MaterialHandle(std::shared_ptr<const MaterialDefinition> materialDefinition, bool forceLocalBlocks = false); // forceLocalBlocks is for engine use only

		MaterialHandle& operator=(const MaterialHandle& other) = default;
		MaterialHandle& operator=(MaterialHandle&& other) = default;

		void bind(int pass, Painter& painter);
		void uploadData(Painter& painter);

		std::weak_ptr<Material> getWeakPtr() { return material; }
		Material& getMaterial() { return *material; }
		bool hasMaterial() const { return !!material; }

		bool operator==(const MaterialHandle& other) const { return material == other.material; }
		bool operator!=(const MaterialHandle& other) const { return material != other.material; }

		bool isCompatibleWith(const MaterialHandle& other) const { return material->isCompatibleWith(*other.material); }

		const MaterialDefinition& getDefinition() const { return material->getDefinition(); }
		
		const std::shared_ptr<const Texture>& getTexture(int textureUnit) const { return material->getTexture(textureUnit); }
		const Vector<MaterialTextureParameter>& getTextureUniforms() const { return material->getTextureUniforms(); }
		const std::vector<std::shared_ptr<const Texture>>& getTextures() const { return material->getTextures(); }
		size_t getNumTextureUnits() const { return material->getNumTextureUnits(); }

		const Vector<MaterialParameter>& getUniforms() const { return material->getUniforms(); }
		const Vector<MaterialDataBlock>& getDataBlocks() const { return material->getDataBlocks(); }

		void setPassEnabled(int pass, bool enabled);
		bool isPassEnabled(int pass) const { return material->isPassEnabled(pass); }

		MaterialHandle& set(const String& name, const std::shared_ptr<const Texture>& texture);
		MaterialHandle& set(const String& name, const std::shared_ptr<Texture>& texture);

		bool hasParameter(const String& name) const { return material->hasParameter(name); }

		template <typename T>
		MaterialHandle& set(const String& name, const T& value)
		{
			copyOnWrite();
			material->set(name, value);
			return *this;
		}

		uint64_t getHash() const { return material->getHash(); }

	private:
		std::shared_ptr<Material> material;

		void copyOnWrite();
	};
}
