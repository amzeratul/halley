#pragma once
#include "halley/core/graphics/blend.h"
#include "halley/data_structures/config_node.h"
#include "halley/resources/resource.h"
#include "halley/maths/range.h"
#include "halley/core/graphics/shader_type.h"

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

	enum class ShaderParameterType : uint8_t
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
		Invalid
	};

	enum class DepthStencilComparisonFunction : uint8_t
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

	enum class TextureSamplerType: uint8_t {
		Texture1D,
		Texture2D,
		Texture3D,
		Depth2D,
		Stencil2D
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

	enum class StencilWriteOperation : uint8_t
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

	enum class CullingMode : uint8_t
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
		String autoVariable;
		std::optional<Range<float>> range;
		float granularity;
		uint32_t offset = 0;
		uint16_t blockNumber = 0;
		bool editable = true;
		ShaderParameterType type;
		ConfigNode defaultValue;

		MaterialUniform();
		MaterialUniform(String name, ShaderParameterType type, std::optional<Range<float>> range = {}, float granularity = 0, bool editable = true, String autoVariable = "", ConfigNode defaultValue = {});

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class MaterialUniformBlock
	{
	public:
		String name;
		Vector<MaterialUniform> uniforms;
		Vector<int> addresses;
		size_t offset = 0;

		MaterialUniformBlock() = default;
		MaterialUniformBlock(String name, Vector<MaterialUniform> uniforms);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		int getAddress(int pass, ShaderType stage) const;
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

	class MaterialDefinition;

	class MaterialTexture
	{
	public:
		String name;
		String defaultTextureName;
		TextureSamplerType samplerType = TextureSamplerType::Texture2D;
		std::shared_ptr<const Texture> defaultTexture;
		Vector<int> addresses;

		MaterialTexture();
		MaterialTexture(String name, String defaultTexture, TextureSamplerType samplerType);

		void loadAddresses(const MaterialDefinition& def);
		unsigned int getAddress(int pass, ShaderType stage) const;

		const String& getName() const { return name; }
		const String& getDefaultTextureName() const { return defaultTextureName; }
		TextureSamplerType getSamplerType() const { return samplerType; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);
	};

	class MaterialDefinition final : public Resource
	{
		friend class Material;
		friend class MaterialParameter;
		friend class MaterialTextureParameter;

	public:
		constexpr static const char* defaultMaterial = "Halley/Sprite";

		MaterialDefinition();
		explicit MaterialDefinition(ResourceLoader& loader);

		void reload(Resource&& resource) override;
		void load(const ConfigNode& node);

		void addPass(const MaterialPass& materialPass);
		int getNumPasses() const;
		const MaterialPass& getPass(int n) const;
		MaterialPass& getPass(int n);
		gsl::span<const MaterialPass> getPasses() const;
		gsl::span<MaterialPass> getPasses();

		const String& getName() const;
		size_t getVertexSize() const;
		size_t getVertexStride() const;
		size_t getVertexPosOffset() const;
		const Vector<MaterialAttribute>& getAttributes() const { return attributes; }
		const Vector<MaterialUniformBlock>& getUniformBlocks() const { return uniformBlocks; }
		const Vector<MaterialTexture>& getTextures() const { return textures; }
		bool hasTexture(const String& name) const;
		const std::shared_ptr<const Texture>& getFallbackTexture() const;
		int getDefaultMask() const;

		static std::unique_ptr<MaterialDefinition> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::MaterialDefinition; }
		
		const Vector<String>& getTags() const { return tags; }
		bool hasTag(const String& tag) const;
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		bool isColumnMajor() const;

	private:
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
		Vector<String> tags;

		void loadUniforms(const ConfigNode& node);
		void loadTextures(const ConfigNode& node);
		void loadAttributes(const ConfigNode& node);
		ShaderParameterType parseParameterType(const String& rawType) const;
		TextureSamplerType parseSamplerType(const String& rawType) const;
	};

	class MaterialDepthStencil
	{
	public:
		MaterialDepthStencil();

		void loadDepth(const ConfigNode& node);
		void loadStencil(const ConfigNode& node);

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		bool isDepthTestEnabled() const { return enableDepthTest; }
		bool isDepthWriteEnabled() const { return enableDepthWrite; }
		bool isStencilTestEnabled() const { return enableStencilTest; }

		uint8_t getStencilReference() const { return stencilReference; }
		uint8_t getStencilWriteMask() const { return stencilWriteMask; }
		uint8_t getStencilReadMask() const { return stencilReadMask; } 

		DepthStencilComparisonFunction getDepthComparisonFunction() const { return depthComparison; }
		DepthStencilComparisonFunction getStencilComparisonFunction() const { return stencilComparison; }

		StencilWriteOperation getStencilOpPass() const { return stencilOpPass; }
		StencilWriteOperation getStencilOpDepthFail() const { return stencilOpDepthFail; }
		StencilWriteOperation getStencilOpStencilFail() const { return stencilOpStencilFail; }

		void setStencilReference(uint8_t value);

		bool operator==(const MaterialDepthStencil& other) const;
		bool operator!=(const MaterialDepthStencil& other) const;

		uint64_t getHash() const;

	private:
		DepthStencilComparisonFunction depthComparison = DepthStencilComparisonFunction::Always;
		DepthStencilComparisonFunction stencilComparison = DepthStencilComparisonFunction::Always;

		StencilWriteOperation stencilOpPass = StencilWriteOperation::Keep;
		StencilWriteOperation stencilOpDepthFail = StencilWriteOperation::Keep;
		StencilWriteOperation stencilOpStencilFail = StencilWriteOperation::Keep;

		uint8_t stencilReference = 0;
		uint8_t stencilWriteMask = 0xFF;
		uint8_t stencilReadMask = 0xFF;

		bool enableDepthTest = false;
		bool enableDepthWrite = false;
		bool enableStencilTest = false;
	};

	class ShaderDefinition;

	class MaterialPass
	{
		friend class Material;

	public:
		MaterialPass();
		explicit MaterialPass(const String& shaderAssetId, const ConfigNode& node);

		BlendType getBlend() const { return blend; }
		void setBlend(BlendType type) { blend = type; }

		Shader& getShader() const { return *shader; }
		void setShader(std::shared_ptr<Shader> shader) { this->shader = std::move(shader); }

		const MaterialDepthStencil& getDepthStencil() const { return depthStencil; }
		MaterialDepthStencil& getDepthStencil() { return depthStencil; }

		CullingMode getCulling() const { return cull; }
		void setCulling(CullingMode mode) { cull = mode; }

		bool isEnabled() const { return enabled; }
		void setEnabled(bool enabled) { this->enabled = enabled; }

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void createShader(ResourceLoader& loader, String name, const Vector<MaterialAttribute>& attributes);
		void replacePixelShader(const MaterialPass& pixelShaderSource, VideoAPI& video);

	private:
		std::shared_ptr<Shader> shader;
		BlendType blend;
		MaterialDepthStencil depthStencil;
		CullingMode cull = CullingMode::None;
		bool enabled = true;
		
		String shaderAssetId;
		std::shared_ptr<ShaderDefinition> shaderDefinition;
	};
}

namespace std
{
    template<>
	struct hash<Halley::MaterialDepthStencil> {
        std::size_t operator()(Halley::MaterialDepthStencil const& v) const noexcept
        {
        	const auto result = v.getHash();
        	static_assert(sizeof(result) == sizeof(std::size_t));
        	return static_cast<std::size_t>(result);
        }
    };
}
