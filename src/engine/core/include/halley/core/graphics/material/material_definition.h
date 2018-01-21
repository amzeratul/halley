#pragma once
#include "halley/core/graphics/blend.h"
#include "halley/resources/resource.h"

namespace Halley
{
	class Deserializer;
	class Serializer;
	class MaterialPass;
	class ResourceLoader;
	class Shader;
	class VideoAPI;
	class Painter;
	class MaterialImporter;
	class MaterialTextureParameter;

	enum class ShaderParameterType
	{
		Float,
		Float2,
		Float3,
		Float4,
		Int,
		Int2,
		Int3,
		Int4,
		Matrix2,
		Matrix3,
		Matrix4,
		Texture2D,
		Invalid
	};

	
	class MaterialUniform
	{
	public:
		String name;
		ShaderParameterType type;

		MaterialUniform();
		MaterialUniform(String name, ShaderParameterType type);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class MaterialUniformBlock
	{
	public:
		String name;
		Vector<MaterialUniform> uniforms;

		MaterialUniformBlock();
		MaterialUniformBlock(const String& name, const Vector<MaterialUniform>& uniforms);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class MaterialAttribute
	{
	public:
		String name;
		ShaderParameterType type;
		int location;
		int offset;

		MaterialAttribute();
		MaterialAttribute(String name, ShaderParameterType type, int location, int offset = 0);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		static size_t getAttributeSize(ShaderParameterType type);
	};

	class MaterialDefinition final : public Resource
	{
		friend class Material;
		friend class MaterialParameter;
		friend class MaterialTextureParameter;
		friend class MaterialImporter;

	public:
		MaterialDefinition();
		explicit MaterialDefinition(ResourceLoader& loader);

		int getNumPasses() const;
		const MaterialPass& getPass(int n) const;
		MaterialPass& getPass(int n);

		const String& getName() const;
		int getVertexStride() const { return vertexStride; }
		int getVertexPosOffset() const { return vertexPosOffset; }
		const Vector<MaterialAttribute>& getAttributes() const { return attributes; }
		const Vector<MaterialUniformBlock>& getUniformBlocks() const { return uniformBlocks; }
		const Vector<String>& getTextures() const { return textures; }

		static std::unique_ptr<MaterialDefinition> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::MaterialDefinition; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		VideoAPI* api = nullptr;

		String name;
		Vector<MaterialPass> passes;
		Vector<String> textures;
		Vector<MaterialUniformBlock> uniformBlocks;
		Vector<MaterialAttribute> attributes;
		int vertexStride = 0;
		int vertexPosOffset = 0;
	};

	class MaterialPass
	{
		friend class Material;

	public:
		MaterialPass();
		MaterialPass(BlendType blend, String shaderAsset);

		BlendType getBlend() const { return blend; }
		Shader& getShader() const { return *shader; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void createShader(ResourceLoader& loader, String name, const Vector<MaterialAttribute>& attributes);

	private:
		std::shared_ptr<Shader> shader;
		BlendType blend;
		
		String shaderAssetId;
	};
}
