#include "halley/resources/resources.h"
#include "halley/api/halley_api_internal.h"
#include "halley/api/halley_api.h"
#include "halley/graphics/shader.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/text/string_converter.h"
#include "halley/file_formats/binary_file.h"
#include "halley/file_formats/config_file.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

namespace {
	constexpr int shaderStageCount = int(ShaderType::NumOfShaderTypes);
}

MaterialUniform::MaterialUniform()
	: granularity(1.0f)
	, type(ShaderParameterType::Invalid)
{}

MaterialUniform::MaterialUniform(String name, ShaderParameterType type, std::optional<Range<float>> range, float granularity, bool editable, String autoVariable, ConfigNode defaultValue)
	: name(std::move(name))
	, autoVariable(std::move(autoVariable))
	, range(range)
	, granularity(granularity)
	, editable(editable)
	, type(type)
	, defaultValue(std::move(defaultValue))
{}

void MaterialUniform::serialize(Serializer& s) const
{
	s << name;
	s << type;
	s << range;
	s << granularity;
	s << editable;
	s << autoVariable;
	s << defaultValue;
}

void MaterialUniform::deserialize(Deserializer& s)
{
	s >> name;
	s >> type;
	s >> range;
	s >> granularity;
	s >> editable;
	s >> autoVariable;
	s >> defaultValue;
}

MaterialUniformBlock::MaterialUniformBlock(String name, Vector<MaterialUniform> uniforms)
	: name(std::move(name))
	, uniforms(std::move(uniforms))
{}

void MaterialUniformBlock::serialize(Serializer& s) const
{
	s << name;
	s << uniforms;
}

void MaterialUniformBlock::deserialize(Deserializer& s)
{
	s >> name;
	s >> uniforms;
}

int MaterialUniformBlock::getAddress(int pass, ShaderType stage) const
{
	return addresses[pass * shaderStageCount + static_cast<int>(stage)];
}

MaterialAttribute::MaterialAttribute()
	: type(ShaderParameterType::Invalid)
    , location(-1)
    , offset(-1)
{}

MaterialAttribute::MaterialAttribute(String name, ShaderParameterType type, int location, int offset)
	: name(name)
	, type(type)
	, location(location)
	, offset(offset)
{}

void MaterialAttribute::serialize(Serializer& s) const
{
	s << name;
	s << type;
	s << semantic;
	s << semanticIndex;
	s << location;
	s << offset;
}

void MaterialAttribute::deserialize(Deserializer& s)
{
	s >> name;
	s >> type;
	s >> semantic;
	s >> semanticIndex;
	s >> location;
	s >> offset;
}

size_t MaterialAttribute::getAttributeSize(ShaderParameterType type)
{
	switch (type) {
		case ShaderParameterType::Float: return 4;
		case ShaderParameterType::Float2: return 8;
		case ShaderParameterType::Float3: return 12;
		case ShaderParameterType::Float4: return 16;
		case ShaderParameterType::Int: return 4;
		case ShaderParameterType::Int2: return 8;
		case ShaderParameterType::Int3: return 12;
		case ShaderParameterType::Int4: return 16;
		case ShaderParameterType::Matrix2: return 16;
		case ShaderParameterType::Matrix3: return 36;
		case ShaderParameterType::Matrix4: return 64;
		case ShaderParameterType::UInt: return 4;
		default: throw Exception("Unknown type: " + toString(int(type)), HalleyExceptions::Resources);
	}
}

size_t MaterialAttribute::getAttributeAlignment(ShaderParameterType type)
{
	switch (type) {
		case ShaderParameterType::Float: return 4;
		case ShaderParameterType::Float2: return 8;
		case ShaderParameterType::Float3: return 16;
		case ShaderParameterType::Float4: return 16;
		case ShaderParameterType::Int: return 4;
		case ShaderParameterType::Int2: return 8;
		case ShaderParameterType::Int3: return 16;
		case ShaderParameterType::Int4: return 16;
		case ShaderParameterType::Matrix2: return 16;
		case ShaderParameterType::Matrix3: return 16;
		case ShaderParameterType::Matrix4: return 16;
		case ShaderParameterType::UInt: return 4;
		default: throw Exception("Unknown type: " + toString(int(type)), HalleyExceptions::Resources);
	}
}

MaterialTexture::MaterialTexture()
{}

MaterialTexture::MaterialTexture(String name, String defaultTexture, TextureSamplerType samplerType)
	: name(std::move(name))
	, defaultTextureName(std::move(defaultTexture))
	, samplerType(samplerType)
{}

void MaterialTexture::loadAddresses(const MaterialDefinition& definition)
{
	const auto& passes = definition.getPasses();
	addresses.resize(passes.size() * shaderStageCount);
	for (size_t i = 0; i < passes.size(); i++) {
		auto& shader = passes[i].getShader();
		for (int j = 0; j < shaderStageCount; ++j) {
			addresses[i * shaderStageCount + j] = shader.getUniformLocation(name, ShaderType(j));
		}
	}
}

unsigned MaterialTexture::getAddress(int pass, ShaderType stage) const
{
	return addresses[pass * shaderStageCount + int(stage)];
}

void MaterialTexture::serialize(Serializer& s) const
{
	s << name;
	s << defaultTextureName;
	s << samplerType;
}

void MaterialTexture::deserialize(Deserializer& s)
{
	s >> name;
	s >> defaultTextureName;
	s >> samplerType;
}

MaterialDefinition::MaterialDefinition() = default;

std::shared_ptr<MaterialDefinition> MaterialDefinition::loadResource(ResourceLoader& loader)
{
	auto result = std::make_shared<MaterialDefinition>(loader);
	
	auto* video = loader.getAPI().video;
	auto& resources = loader.getResources();

	Concurrent::execute(Executors::getVideoAux(), [result, video, &resources]()
	{
		int i = 0;
		for (auto& p: result->passes) {
			p.createShader(*video, resources, result->name + "/pass" + toString(i++), result->attributes);
		}
		result->initialize(*video);
		result->doneLoading();
	});

	return result;
}

MaterialDefinition::MaterialDefinition(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	s >> *this;

	startLoading();

	auto& resources = loader.getResources();

	fallbackTexture = resources.get<Texture>("whitebox.png");
	for (auto& tex: textures) {
		if (!tex.defaultTextureName.isEmpty() && !tex.defaultTextureName.startsWith("$")) {
			tex.defaultTexture = resources.get<Texture>(tex.defaultTextureName);
		}
	}
}

void MaterialDefinition::initialize(VideoAPI& video)
{
	columnMajor = video.isColumnMajor();

	// Load textures
	for (auto& tex: textures) {
		tex.loadAddresses(*this);
	}

	// Load uniform blocks
	uint16_t blockNumber = 0;
	for (auto& uniformBlock: uniformBlocks) {
		uniformBlock.addresses.resize(getNumPasses() * shaderStageCount);
		for (int i = 0; i < int(passes.size()); ++i) {
			auto& shader = passes[i].getShader();
			for (int j = 0; j < shaderStageCount; ++j) {
				uniformBlock.addresses[i * shaderStageCount + j] = shader.getBlockLocation(uniformBlock.name, static_cast<ShaderType>(j));
			}
		}

		size_t curOffset = 0;
		for (auto& uniform: uniformBlock.uniforms) {
			const auto size = MaterialAttribute::getAttributeSize(uniform.type);
			const auto alignment = MaterialAttribute::getAttributeAlignment(uniform.type);

			curOffset = alignUp(curOffset, alignment);

			uniform.blockNumber = blockNumber;
			if (uniform.predefinedOffset) {
				curOffset = uniform.offset;
			} else {
				uniform.offset = static_cast<uint32_t>(curOffset);
			}
			curOffset += size;
		}
		uniformBlock.offset = curOffset;
		++blockNumber;
	}

	updateUniformBlocks();
}

void MaterialDefinition::reload(Resource&& resource)
{
	auto& other = dynamic_cast<MaterialDefinition&>(resource);
	*this = std::move(other);
}

void MaterialDefinition::load(const ConfigNode& root)
{
	// Load name
	name = root["name"].asString("Unknown");
	defaultMask = root["defaultMask"].asInt(1);
	tags = root["tags"].asVector<String>({});

	// Load attributes & uniforms
	if (root.hasKey("attributes")) {
		loadAttributes(root["attributes"]);
	}
	if (root.hasKey("uniforms")) {
		loadUniforms(root["uniforms"]);
	}
	if (root.hasKey("textures")) {
		loadTextures(root["textures"]);
	}
}

int MaterialDefinition::getNumPasses() const
{
	return int(passes.size());
}

const MaterialPass& MaterialDefinition::getPass(int n) const
{
	return passes[n];
}

MaterialPass& MaterialDefinition::getPass(int n)
{
	return passes[n];
}

gsl::span<const MaterialPass> MaterialDefinition::getPasses() const
{
	return passes;
}

gsl::span<MaterialPass> MaterialDefinition::getPasses()
{
	return passes;
}

void MaterialDefinition::setName(String name)
{
	this->name = std::move(name);
}

const String& MaterialDefinition::getName() const
{
	return name;
}

void MaterialDefinition::setDefaultMask(int mask)
{
	defaultMask = mask;
}

size_t MaterialDefinition::getVertexSize() const
{
	return size_t(vertexSize);
}

size_t MaterialDefinition::getVertexStride() const
{
	if (vertexSize % 16 != 0) {
		return size_t(vertexSize + 16 - vertexSize % 16);
	} else {
		return size_t(vertexSize);
	}
}

size_t MaterialDefinition::getVertexPosOffset() const
{
	return size_t(vertexPosOffset);
}

void MaterialDefinition::setAttributes(Vector<MaterialAttribute> attributes)
{
	this->attributes = std::move(attributes);
	assignAttributeOffsets();
}

void MaterialDefinition::setUniformBlocks(Vector<MaterialUniformBlock> uniformBlocks)
{
	this->uniformBlocks = std::move(uniformBlocks);
	updateUniformBlocks();
}

void MaterialDefinition::setTextures(Vector<MaterialTexture> textures)
{
	this->textures = std::move(textures);
}

Vector<String> MaterialDefinition::getTextureNames() const
{
	Vector<String> result;
	result.reserve(textures.size());
	for (auto& t: textures) {
		result.push_back(t.name);
	}
	return result;
}

bool MaterialDefinition::hasTexture(const String& name) const
{
	return std_ex::contains_if(textures, [&] (const auto& t) { return t.name == name; });
}

const std::shared_ptr<const Texture>& MaterialDefinition::getFallbackTexture() const
{
	return fallbackTexture;
}

int MaterialDefinition::getDefaultMask() const
{
	return defaultMask;
}

void MaterialDefinition::addPass(MaterialPass materialPass)
{
	passes.push_back(std::move(materialPass));
}

void MaterialDefinition::setTags(Vector<String> tags)
{
	this->tags = std::move(tags);
}

bool MaterialDefinition::hasTag(std::string_view tag) const
{
	return std_ex::contains(tags, tag);
}

void MaterialDefinition::serialize(Serializer& s) const
{
	s << name;
	s << passes;
	s << textures;
	s << uniformBlocks;
	s << attributes;
	s << vertexSize;
	s << vertexPosOffset;
	s << defaultMask;
	s << tags;
}

void MaterialDefinition::deserialize(Deserializer& s)
{
	s >> name;
	s >> passes;
	s >> textures;
	s >> uniformBlocks;
	s >> attributes;
	s >> vertexSize;
	s >> vertexPosOffset;
	s >> defaultMask;
	s >> tags;
}

bool MaterialDefinition::isColumnMajor() const
{
	return columnMajor;
}

bool MaterialDefinition::hasAutoVariables() const
{
	return autoVariables;
}

std::shared_ptr<const Material> MaterialDefinition::getMaterial() const
{
	waitForLoad();
	auto m = material.lock();
	if (!m) {
		m = makeMaterial();
		material = m;
	}
	return m;
}

std::shared_ptr<const Material> MaterialDefinition::makeMaterial() const
{
	return std::make_shared<Material>(shared_from_this());
}

void MaterialDefinition::updateUniformBlocks()
{
	autoVariables = false;
	for (const auto& block: uniformBlocks) {
		for (const auto& uniform : block.uniforms) {
			if (!uniform.autoVariable.isEmpty()) {
				autoVariables = true;
			}
		}
	}
}

void MaterialDefinition::loadUniforms(const ConfigNode& node)
{
	for (auto& blockEntry : node.asSequence()) {
		for (auto& it: blockEntry.asMap()) {
			const String& blockName = it.first;
			auto& uniformNodes = it.second;

			Vector<MaterialUniform> uniforms;
			for (auto& uniformEntry: uniformNodes.asSequence()) {
				if (uniformEntry.hasKey("name")) {
					const auto name = uniformEntry["name"].asString();
					const auto type = parseParameterType(uniformEntry["type"].asString());
					std::optional<Range<float>> range;
					if (uniformEntry.hasKey("range")) {
						range = uniformEntry["range"].asFloatRange();
					}
					const auto granularity = uniformEntry["granularity"].asFloat(1);
					const bool editable = uniformEntry["canEdit"].asBool(true);
					const auto autoVariable = uniformEntry["autoVariable"].asString("");
					uniforms.push_back(MaterialUniform(name, type, range, granularity, editable, autoVariable, ConfigNode(uniformEntry["defaultValue"])));
				} else {
					for (auto& uit: uniformEntry.asMap()) {
						String uniformName = uit.first;
						ShaderParameterType type = parseParameterType(uit.second.asString());
						uniforms.push_back(MaterialUniform(uniformName, type));
					}
				}
			}
			
			uniformBlocks.push_back(MaterialUniformBlock(blockName, uniforms));
		}
	}

	updateUniformBlocks();
}

void MaterialDefinition::loadTextures(const ConfigNode& node)
{
	for (auto& attribEntry: node.asSequence()) {
		MaterialTexture tex;
		
		if (attribEntry.hasKey("name")) {
			// New format
			tex.name = attribEntry["name"].asString();
			tex.samplerType = parseSamplerType(attribEntry["sampler"].asString("sampler2D"));
			tex.defaultTextureName = attribEntry["defaultTexture"].asString("");
		} else {
			// Old format
			for (auto& it : attribEntry.asMap()) {
				tex.name = it.first;
				tex.samplerType = parseSamplerType(it.second.asString());
			}
		}

		textures.push_back(tex);
	}
}

void MaterialDefinition::loadAttributes(const ConfigNode& node)
{
	for (const auto& attribEntry: node.asSequence()) {
		const ShaderParameterType type = parseParameterType(attribEntry["type"].asString());
		String semantic = attribEntry["semantic"].asString();
		int semanticIndex = 0;
		if (semantic.right(1).isInteger()) {
			semanticIndex = semantic.right(1).toInteger();
			semantic = semantic.left(semantic.size() - 1);
		}

		attributes.push_back(MaterialAttribute());
		auto& a = attributes.back();
		a.name = attribEntry["name"].asString("");
		a.type = type;
		a.semantic = semantic;
		a.semanticIndex = semanticIndex;
		a.isVertexPos = attribEntry["special"].asString("") == "vertPos";
	}

	assignAttributeOffsets();
}

void MaterialDefinition::assignAttributeOffsets()
{
	int location = 0;
	vertexSize = 0;

	for (auto& a: attributes) {
		a.location = location++;
		a.offset = vertexSize;

		const int size = int(MaterialAttribute::getAttributeSize(a.type));
		vertexSize += size;

		if (a.isVertexPos) {
			vertexPosOffset = a.offset;
		}
	}
}

ShaderParameterType MaterialDefinition::parseParameterType(const String& rawType) const
{
	if (rawType == "float") {
		return ShaderParameterType::Float;
	} else if (rawType == "float2" || rawType == "vec2") {
		return ShaderParameterType::Float2;
	} else if (rawType == "float3" || rawType == "vec3") {
		return ShaderParameterType::Float3;
	} else if (rawType == "float4" || rawType == "vec4") {
		return ShaderParameterType::Float4;
	} else if (rawType == "int") {
		return ShaderParameterType::Int;
	} else if (rawType == "int2") {
		return ShaderParameterType::Int2;
	} else if (rawType == "int3") {
		return ShaderParameterType::Int3;
	} else if (rawType == "int4") {
		return ShaderParameterType::Int4;
	} else if (rawType == "float4x4" || rawType == "mat4") {
		return ShaderParameterType::Matrix4;
	} else {
		throw Exception("Unknown attribute type: " + rawType, HalleyExceptions::Resources);
	}
}

TextureSamplerType MaterialDefinition::parseSamplerType(const String& rawType) const
{
	if (rawType == "sampler1D") {
		return TextureSamplerType::Texture1D;
	} else if (rawType == "sampler2D") {
		return TextureSamplerType::Texture2D;
	} else if (rawType == "sampler3D") {
		return TextureSamplerType::Texture3D;
	} else if (rawType == "depth2D") {
		return TextureSamplerType::Depth2D;
	} else if (rawType == "stencil2D") {
		return TextureSamplerType::Stencil2D;
	} else {
		throw Exception("Unknown sampler type: " + rawType, HalleyExceptions::Resources);
	}
}

MaterialDepthStencil::MaterialDepthStencil()
{
}

void MaterialDepthStencil::loadDepth(const ConfigNode& node)
{
	enableDepthTest = node["test"].asBool(false);
	enableDepthWrite = node["write"].asBool(false);
	depthComparison = fromString<DepthStencilComparisonFunction>(node["comparison"].asString("Always"));
}

void MaterialDepthStencil::loadStencil(const ConfigNode& node)
{
	enableStencilTest = node["test"].asBool(false);
	stencilComparison = fromString<DepthStencilComparisonFunction>(node["comparison"].asString("Always"));

	stencilReference = node["reference"].asInt(0);
	stencilReadMask = node["readMask"].asInt(0xFF);
	stencilWriteMask = node["writeMask"].asInt(0xFF);

	stencilOpPass = fromString<StencilWriteOperation>(node["opPass"].asString("Keep"));
	stencilOpDepthFail = fromString<StencilWriteOperation>(node["opDepthFail"].asString("Keep"));
	stencilOpStencilFail = fromString<StencilWriteOperation>(node["opStencilFail"].asString("Keep"));
}

void MaterialDepthStencil::serialize(Serializer& s) const
{
	s << enableDepthTest;
	s << enableDepthWrite;
	s << enableStencilTest;

	s << stencilReference;
	s << stencilWriteMask;
	s << stencilReadMask;

	s << depthComparison;
	s << stencilComparison;

	s << stencilOpPass;
	s << stencilOpDepthFail;
	s << stencilOpStencilFail;
}

void MaterialDepthStencil::deserialize(Deserializer& s)
{
	s >> enableDepthTest;
	s >> enableDepthWrite;
	s >> enableStencilTest;

	s >> stencilReference;
	s >> stencilWriteMask;
	s >> stencilReadMask;

	s >> depthComparison;
	s >> stencilComparison;

	s >> stencilOpPass;
	s >> stencilOpDepthFail;
	s >> stencilOpStencilFail;
}

void MaterialDepthStencil::setStencilReference(uint8_t value)
{
	stencilReference = value;
}

bool MaterialDepthStencil::operator==(const MaterialDepthStencil& other) const
{
	return depthComparison == other.depthComparison
		&& stencilComparison == other.stencilComparison
		&& stencilOpPass == other.stencilOpPass
		&& stencilOpDepthFail == other.stencilOpDepthFail
		&& stencilOpStencilFail == other.stencilOpStencilFail
		&& stencilReference == other.stencilReference
		&& stencilWriteMask == other.stencilWriteMask
		&& stencilReadMask == other.stencilReadMask
		&& enableDepthTest == other.enableDepthTest
		&& enableDepthWrite == other.enableDepthWrite
		&& enableStencilTest == other.enableStencilTest;
}

bool MaterialDepthStencil::operator!=(const MaterialDepthStencil& other) const
{
	return depthComparison != other.depthComparison
		|| stencilComparison != other.stencilComparison
		|| stencilOpPass != other.stencilOpPass
		|| stencilOpDepthFail != other.stencilOpDepthFail
		|| stencilOpStencilFail != other.stencilOpStencilFail
		|| stencilReference != other.stencilReference
		|| stencilWriteMask != other.stencilWriteMask
		|| stencilReadMask != other.stencilReadMask
		|| enableDepthTest != other.enableDepthTest
		|| enableDepthWrite != other.enableDepthWrite
		|| enableStencilTest != other.enableStencilTest;
}

uint64_t MaterialDepthStencil::getHash() const
{
	union {
		uint64_t hash;
		struct {
			uint8_t dc : 4;
			uint8_t sc : 4;
			uint8_t sop : 4;
			uint8_t sodf : 4;
			uint8_t sosf : 4;
			uint8_t edt : 1;
			uint8_t edw : 1;
			uint8_t est : 1;
			uint8_t sr;
			uint8_t swm;
			uint8_t srm;
		};
	} v;
	v.hash = 0;

	v.dc = static_cast<uint8_t>(depthComparison);
	v.sc = static_cast<uint8_t>(stencilComparison);

	v.sop = static_cast<uint8_t>(stencilOpPass);
	v.sodf = static_cast<uint8_t>(stencilOpDepthFail);
	v.sosf = static_cast<uint8_t>(stencilOpStencilFail);

	v.sr = stencilReference;
	v.swm = stencilWriteMask;
	v.srm = stencilReadMask;

	v.edt = enableDepthTest;
	v.edw = enableDepthWrite;
	v.est = enableStencilTest;

	return v.hash;
}


MaterialPass::MaterialPass() = default;

MaterialPass::MaterialPass(const String& shaderAssetId, const ConfigNode& node)
	: shaderAssetId(shaderAssetId)
{
	auto blendParts = node["blend"].asString("Opaque").split(' ');
	for (auto& part: blendParts) {
		if (part == "Premultiplied") {
			blend.premultiplied = true;
		} else {
			blend.mode = fromString<BlendMode>(part);
		}
	}
	
	cull = fromString<CullingMode>(node["cull"].asString("None"));
	enabled = node["enabled"].asBool(true);

	if (node.hasKey("depth")) {
		depthStencil.loadDepth(node["depth"]);
	}
	if (node.hasKey("stencil")) {
		depthStencil.loadStencil(node["stencil"]);
	}
}

MaterialPass::MaterialPass(std::shared_ptr<Shader> shader, BlendMode, MaterialDepthStencil depthStencil, CullingMode cull)
	: shader(std::move(shader))
	, depthStencil(std::move(depthStencil))
	, cull(cull)
{
}

void MaterialPass::serialize(Serializer& s) const
{
	s << blend;
	s << shaderAssetId;
	s << depthStencil;
	s << cull;
	s << enabled;
}

void MaterialPass::deserialize(Deserializer& s)
{
	s >> blend;
	s >> shaderAssetId;
	s >> depthStencil;
	s >> cull;
	s >> enabled;
}

void MaterialPass::createShader(VideoAPI& video, Resources& resources, String name, const Vector<MaterialAttribute>& attributes)
{
	auto shaderData = resources.get<ShaderFile>(shaderAssetId + ":" + video.getShaderLanguage());

	auto definition = std::make_shared<ShaderDefinition>();
	definition->name = name;
	definition->vertexAttributes = attributes;
	definition->shaders = shaderData->shaders;

	shader = video.createShader(*definition);

	if (resources.getOptions().retainShaderData) {
		shaderDefinition = std::move(definition);
	}
}

void MaterialPass::replacePixelShader(const MaterialPass& pixelShaderSource, VideoAPI& video)
{
	shaderDefinition = std::make_shared<ShaderDefinition>(*shaderDefinition);
	shaderDefinition->shaders[ShaderType::Pixel] = pixelShaderSource.shaderDefinition->shaders[ShaderType::Pixel];
	shader = video.createShader(*shaderDefinition);
}
