#pragma once

#include <memory>
#include "halley/text/halleystring.h"
#include "halley/core/graphics/texture.h"
#include <gsl/gsl>

namespace Halley
{
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

	class MaterialDataBlock
	{
		friend class Material;

	public:
		MaterialDataBlock();
		MaterialDataBlock(const String& name, const MaterialDefinition& def);
		MaterialDataBlock(const MaterialDataBlock& other);
		MaterialDataBlock(MaterialDataBlock&& other) noexcept;

		MaterialConstantBuffer& getConstantBuffer() const;
		int getAddress(int pass) const;
		gsl::span<const gsl::byte> getData() const;

	private:
		std::unique_ptr<MaterialConstantBuffer> constantBuffer;
		Bytes data;
		Vector<int> addresses;
		bool dirty = true;

		void setUniform(size_t offset, ShaderParameterType type, void* data);
		void upload(int passNumber, VideoAPI* api);
	};
	
	class Material
	{
		friend class MaterialParameter;

	public:
		Material(const Material& other);
		explicit Material(std::shared_ptr<const MaterialDefinition> materialDefinition);
		void bind(int pass, Painter& painter);
		static void resetBindCache();

		const MaterialDefinition& getDefinition() const { return *materialDefinition; }

		std::shared_ptr<Material> clone() const;
		
		const std::shared_ptr<const Texture>& getMainTexture() const;
		const std::shared_ptr<const Texture>& getTexture(int textureUnit) const;
		const Vector<MaterialTextureParameter>& getTextureUniforms() const;

		const Vector<MaterialParameter>& getUniforms() const;
		const Vector<MaterialDataBlock>& getDataBlocks() const;

		Material& set(const String& name, const std::shared_ptr<const Texture>& texture);
		Material& set(const String& name, const std::shared_ptr<Texture>& texture);

		template <typename T>
		Material& set(const String& name, const T& value)
		{
			getParameter(name) = value;
			return *this;
		}

	private:
		std::shared_ptr<const MaterialDefinition> materialDefinition;
		
		Vector<MaterialParameter> uniforms;
		Vector<MaterialTextureParameter> textureUniforms;
		Vector<MaterialDataBlock> dataBlocks;
		std::vector<std::shared_ptr<const Texture>> textures;
		bool dirty = true;

		void initUniforms();
		MaterialParameter& getParameter(const String& name);

		void setUniform(int blockNumber, size_t offset, ShaderParameterType type, void* data);
	};
}
