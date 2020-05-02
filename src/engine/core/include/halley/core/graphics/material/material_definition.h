#pragma once
#include "halley/core/graphics/blend.h"
#include "halley/resources/resource.h"

namespace Halley
{
	class ConfigNode;
	class Deserializer;
	class Serializer;
	class MaterialPass;
	class ResourceLoader;
	class Shader;
	class VideoAPI;
	class Painter;
	class MaterialImporter;
	class MaterialTextureParameter;
	class Texture;

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

	enum class DepthStencilComparisonFunction
	{
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always
	};

	template <>
	struct EnumNames<DepthStencilComparisonFunction> {
		constexpr std::array<const char*, 8> operator()() const {
			return{{
				"Never",
				"Less",
				"Equal",
				"LessEqual",
				"Greater",
				"NotEqual",
				"GreaterEqual",
				"Always"
			}};
		}
	};

	enum class StencilWriteOperation
	{
		Keep,
		Zero,
		Replace,
		IncrementClamp,
		DecrementClamp,
		Invert,
		IncrementWrap,
		DecrementWrap
	};

	template <>
	struct EnumNames<StencilWriteOperation> {
		constexpr std::array<const char*, 8> operator()() const {
			return{{
				"Keep",
				"Zero",
				"Replace",
				"IncrementClamp",
				"DecrementClamp",
				"Invert",
				"IncrementWrap",
				"DecrementWrap"
			}};
		}
	};

	enum class CullingMode
	{
		None,
		Front,
		Back
	};

	template <>
	struct EnumNames<CullingMode> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"None",
				"Front",
				"Back"
			}};
		}
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
		String semantic;
		int semanticIndex = 0;
		int location = 0;
		int offset = 0;

		MaterialAttribute();
		MaterialAttribute(String name, ShaderParameterType type, int location, int offset = 0);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		static size_t getAttributeSize(ShaderParameterType type);
	};

	class MaterialTexture
	{
	public:
		String name;
		String defaultTextureName;
		ShaderParameterType samplerType = ShaderParameterType::Texture2D;
		std::shared_ptr<const Texture> defaultTexture;

		MaterialTexture();
		MaterialTexture(String name, String defaultTexture, ShaderParameterType samplerType);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class MaterialDefinition final : public Resource
	{
		friend class Material;
		friend class MaterialParameter;
		friend class MaterialTextureParameter;

	public:
		MaterialDefinition();
		explicit MaterialDefinition(ResourceLoader& loader);

		void reload(Resource resource) override;
		void load(const ConfigNode& node);

		void addPass(const MaterialPass& materialPass);
		int getNumPasses() const;
		const MaterialPass& getPass(int n) const;
		MaterialPass& getPass(int n);

		const String& getName() const;
		size_t getVertexSize() const;
		size_t getVertexStride() const;
		size_t getVertexPosOffset() const;
		const Vector<MaterialAttribute>& getAttributes() const { return attributes; }
		const Vector<MaterialUniformBlock>& getUniformBlocks() const { return uniformBlocks; }
		const Vector<MaterialTexture>& getTextures() const { return textures; }
		const std::shared_ptr<const Texture>& getFallbackTexture() const;
		int getDefaultMask() const;

		static std::unique_ptr<MaterialDefinition> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::MaterialDefinition; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		bool isColumnMajor() const;

	private:
		VideoAPI* api = nullptr;

		String name;
		Vector<MaterialPass> passes;
		Vector<MaterialTexture> textures;
		Vector<MaterialUniformBlock> uniformBlocks;
		Vector<MaterialAttribute> attributes;
		int vertexSize = 0;
		int vertexPosOffset = 0;
		int defaultMask = 1;
		bool columnMajor = false;

		std::shared_ptr<const Texture> fallbackTexture;

		void loadUniforms(const ConfigNode& node);
		void loadTextures(const ConfigNode& node);
		void loadAttributes(const ConfigNode& node);
		ShaderParameterType parseParameterType(String rawType) const;
	};

	class MaterialDepthStencil
	{
	public:
		MaterialDepthStencil();

		void loadDepth(const ConfigNode& node);
		void loadStencil(const ConfigNode& node);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		bool isDepthTestEnabled() const;
		bool isDepthWriteEnabled() const;
		bool isStencilTestEnabled() const;

		int getStencilReference() const;
		int getStencilWriteMask() const;
		int getStencilReadMask() const;

		DepthStencilComparisonFunction getDepthComparisonFunction() const;
		DepthStencilComparisonFunction getStencilComparisonFunction() const;

		StencilWriteOperation getStencilOpPass() const;
		StencilWriteOperation getStencilOpDepthFail() const;
		StencilWriteOperation getStencilOpStencilFail() const;

		bool operator==(const MaterialDepthStencil& other) const;
		bool operator!=(const MaterialDepthStencil& other) const;

	private:
		DepthStencilComparisonFunction depthComparison = DepthStencilComparisonFunction::Always;
		DepthStencilComparisonFunction stencilComparison = DepthStencilComparisonFunction::Always;

		StencilWriteOperation stencilOpPass = StencilWriteOperation::Keep;
		StencilWriteOperation stencilOpDepthFail = StencilWriteOperation::Keep;
		StencilWriteOperation stencilOpStencilFail = StencilWriteOperation::Keep;

		int stencilReference = 0;
		int stencilWriteMask = 0xFF;
		int stencilReadMask = 0xFF;

		bool enableDepthTest = false;
		bool enableDepthWrite = false;
		bool enableStencilTest = false;
	};

	class MaterialPass
	{
		friend class Material;

	public:
		MaterialPass();
		explicit MaterialPass(const String& shaderAssetId, const ConfigNode& node);

		BlendType getBlend() const { return blend; }
		Shader& getShader() const { return *shader; }
		const MaterialDepthStencil& getDepthStencil() const { return depthStencil; }
		CullingMode getCulling() const { return cull; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void createShader(ResourceLoader& loader, String name, const Vector<MaterialAttribute>& attributes);

	private:
		std::shared_ptr<Shader> shader;
		BlendType blend;
		MaterialDepthStencil depthStencil;
		CullingMode cull = CullingMode::None;
		
		String shaderAssetId;
	};
}
