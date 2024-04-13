#pragma once
#include "halley/graphics/blend.h"
#include "halley/data_structures/config_node.h"
#include "halley/resources/resource.h"
#include "halley/maths/range.h"
#include "halley/graphics/shader_type.h"

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
	class Material;

	enum class ShaderParameterType : uint8_t
	{
		Invalid,
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
		UInt
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
		Invalid,
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
		bool predefinedOffset = false;
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
		uint8_t bindingMask = 0;
		std::optional<int> bindingPoint;
		bool shared = false;

		MaterialUniformBlock() = default;
		MaterialUniformBlock(String name, gsl::span<const ShaderType> bindings, bool isShared, Vector<MaterialUniform> uniforms, std::optional<int> bindingPoint);

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
		static size_t getAttributeAlignment(ShaderParameterType type);
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

	class MaterialDefinition final : public AsyncResource, public std::enable_shared_from_this<MaterialDefinition>
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
		void initialize(VideoAPI& video);

		void addPass(MaterialPass materialPass);
		int getNumPasses() const;
		const MaterialPass& getPass(int n) const;
		MaterialPass& getPass(int n);
		gsl::span<const MaterialPass> getPasses() const;
		gsl::span<MaterialPass> getPasses();

		void setName(String name);
		const String& getName() const;
		void setDefaultMask(int mask);
		int getDefaultMask() const;

		size_t getVertexSize() const;
		size_t getVertexStride() const;
		size_t getObjectSize() const;
		size_t getObjectStride() const;

		void setVertexAttributes(Vector<MaterialAttribute> attributes);
		const Vector<MaterialAttribute>& getVertexAttributes() const { return vertexAttributes; }
		void setObjectAttributes(Vector<MaterialAttribute> attributes);
		const Vector<MaterialAttribute>& getObjectAttributes() const { return objectAttributes; }

		void setUniformBlocks(Vector<MaterialUniformBlock> uniformBlocks);
		const Vector<MaterialUniformBlock>& getUniformBlocks() const { return uniformBlocks; }
		void setTextures(Vector<MaterialTexture> textures);
		const Vector<MaterialTexture>& getTextures() const { return textures; }
		Vector<String> getTextureNames() const;

		bool hasTexture(const String& name) const;
		const std::shared_ptr<const Texture>& getFallbackTexture() const;

		static std::shared_ptr<MaterialDefinition> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::MaterialDefinition; }

		void setTags(Vector<String> tags);
		const Vector<String>& getTags() const { return tags; }
		bool hasTag(std::string_view tag) const;
		
		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		bool isColumnMajor() const;
		bool hasAutoVariables() const;

		std::shared_ptr<const Material> getMaterial() const;
		NOINLINE virtual std::shared_ptr<const Material> makeMaterial() const;

	private:
		String name;
		Vector<MaterialPass> passes;
		Vector<MaterialTexture> textures;
		Vector<MaterialUniformBlock> uniformBlocks;
		Vector<MaterialAttribute> vertexAttributes;
		Vector<MaterialAttribute> objectAttributes;
		int vertexSize = 0;
		int objectSize = 0;
		int defaultMask = 1;
		bool columnMajor = false;
		bool autoVariables = false;

		std::shared_ptr<const Texture> fallbackTexture;
		Vector<String> tags;
		mutable std::weak_ptr<const Material> material;

		void updateUniformBlocks();
		void loadUniforms(const ConfigNode& node);
		void loadTextures(const ConfigNode& node);
		void loadAttributes(Vector<MaterialAttribute>& attributes, const ConfigNode& node);
		void assignAttributeOffsets();
		int assignAttributeOffsets(Vector<MaterialAttribute>& attributes) const;
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
		MaterialPass(const String& shaderAssetId, const ConfigNode& node);
		explicit MaterialPass(std::shared_ptr<Shader> shader, BlendMode = BlendMode::Opaque, MaterialDepthStencil depthStencil = {}, CullingMode cull = CullingMode::None);

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

		void createShader(VideoAPI& video, Resources& resources, String name, const Vector<MaterialAttribute>& attributes);
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
			if constexpr (sizeof(result) == sizeof(std::size_t)) {
				return static_cast<std::size_t>(result);
			} else {
				return Halley::combineHash(static_cast<size_t>(result >> 32), static_cast<size_t>(result));
			}
        }
    };
}
