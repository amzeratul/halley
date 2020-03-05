#include "resources/resources.h"
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/shader.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/text/string_converter.h"
#include "halley/file_formats/binary_file.h"
#include "halley/file_formats/config_file.h"

using namespace Halley;

MaterialUniform::MaterialUniform()
	: type(ShaderParameterType::Invalid)
{}

MaterialUniform::MaterialUniform(String name, ShaderParameterType type)
	: name(name)
	, type(type)
{}

void MaterialUniform::serialize(Serializer& s) const
{
	s << name;
	s << type;
}

void MaterialUniform::deserialize(Deserializer& s)
{
	s >> name;
	s >> type;
}

MaterialUniformBlock::MaterialUniformBlock() {}

MaterialUniformBlock::MaterialUniformBlock(const String& name, const Vector<MaterialUniform>& uniforms)
	: name(name)
	, uniforms(uniforms)
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
	s << location;
	s << offset;
}

void MaterialAttribute::deserialize(Deserializer& s)
{
	s >> name;
	s >> type;
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
		case ShaderParameterType::Texture2D: return 4;
		default: throw Exception("Unknown type: " + toString(int(type)), HalleyExceptions::Resources);
	}
}

MaterialDefinition::MaterialDefinition() {}

MaterialDefinition::MaterialDefinition(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	s >> *this;

	api = loader.getAPI().video;
	int i = 0;
	for (auto& p: passes) {
		p.createShader(loader, name + "/pass" + toString(i++), attributes);
	}
	columnMajor = api->isColumnMajor();

	fallbackTexture = loader.getResources().get<Texture>("whitebox.png");
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

const String& MaterialDefinition::getName() const
{
	return name;
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

const std::shared_ptr<const Texture>& MaterialDefinition::getFallbackTexture() const
{
	return fallbackTexture;
}

void MaterialDefinition::addPass(const MaterialPass& materialPass)
{
	passes.push_back(materialPass);
}

std::unique_ptr<MaterialDefinition> MaterialDefinition::loadResource(ResourceLoader& loader)
{
	return std::make_unique<MaterialDefinition>(loader);
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
}

bool MaterialDefinition::isColumnMajor() const
{
	return columnMajor;
}

void MaterialDefinition::loadUniforms(const ConfigNode& node)
{
	for (auto& attribEntry : node.asSequence()) {
		for (auto& it: attribEntry.asMap()) {
			const String& blockName = it.first;
			auto& uniformNodes = it.second;

			std::vector<MaterialUniform> uniforms;
			for (auto& uniformEntry: uniformNodes.asSequence()) {
				for (auto& uit: uniformEntry.asMap()) {
					String uniformName = uit.first;
					ShaderParameterType type = parseParameterType(uit.second.asString());
					uniforms.push_back(MaterialUniform(uniformName, type));
				}
			}
			
			uniformBlocks.push_back(MaterialUniformBlock(blockName, uniforms));
		}
	}
}

void MaterialDefinition::loadTextures(const ConfigNode& node)
{
	for (auto& attribEntry: node.asSequence()) {
		for (auto& it: attribEntry.asMap()) {
			String name = it.first;
			ShaderParameterType type = parseParameterType(it.second.asString());
			if (type != ShaderParameterType::Texture2D) {
				throw Exception("Texture \"" + name + "\" must be sampler2D", HalleyExceptions::Resources);
			}

			textures.push_back(name);
		}
	}
}

void MaterialDefinition::loadAttributes(const ConfigNode& node)
{
	int location = int(attributes.size());
	int offset = vertexSize;

	for (auto& attribEntry: node.asSequence()) {
		for (auto& it: attribEntry.asMap()) {
			String name = it.first;
			ShaderParameterType type = parseParameterType(it.second.asString());

			attributes.push_back(MaterialAttribute());
			auto& a = attributes.back();
			a.name = name;
			a.type = type;
			a.location = location++;
			a.offset = offset;

			int size = int(MaterialAttribute::getAttributeSize(type));
			offset += size;

			if (a.name == "a_vertPos") {
				vertexPosOffset = a.offset;
			}
		}
	}

	vertexSize = offset;
}

ShaderParameterType MaterialDefinition::parseParameterType(String rawType) const
{
	if (rawType == "float") {
		return ShaderParameterType::Float;
	} else if (rawType == "vec2") {
		return ShaderParameterType::Float2;
	} else if (rawType == "vec3") {
		return ShaderParameterType::Float3;
	} else if (rawType == "vec4") {
		return ShaderParameterType::Float4;
	} else if (rawType == "mat4") {
		return ShaderParameterType::Matrix4;
	} else if (rawType == "sampler2D") {
		return ShaderParameterType::Texture2D;
	} else {
		throw Exception("Unknown attribute type: " + rawType, HalleyExceptions::Resources);
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

bool MaterialDepthStencil::isDepthTestEnabled() const
{
	return enableDepthTest;
}

bool MaterialDepthStencil::isDepthWriteEnabled() const
{
	return enableDepthWrite;
}

bool MaterialDepthStencil::isStencilTestEnabled() const
{
	return enableStencilTest;
}

int MaterialDepthStencil::getStencilReference() const
{
	return stencilReference;
}

int MaterialDepthStencil::getStencilWriteMask() const
{
	return stencilWriteMask;
}

int MaterialDepthStencil::getStencilReadMask() const
{
	return stencilReadMask;
}

DepthStencilComparisonFunction MaterialDepthStencil::getDepthComparisonFunction() const
{
	return depthComparison;
}

DepthStencilComparisonFunction MaterialDepthStencil::getStencilComparisonFunction() const
{
	return stencilComparison;
}

StencilWriteOperation MaterialDepthStencil::getStencilOpPass() const
{
	return stencilOpPass;
}

StencilWriteOperation MaterialDepthStencil::getStencilOpDepthFail() const
{
	return stencilOpDepthFail;
}

StencilWriteOperation MaterialDepthStencil::getStencilOpStencilFail() const
{
	return stencilOpStencilFail;
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


MaterialPass::MaterialPass()
	: blend(BlendType::Undefined)
{}

MaterialPass::MaterialPass(const String& shaderAssetId, const ConfigNode& node)
	: shaderAssetId(shaderAssetId)
{
	String blendName = node["blend"].asString("Opaque");
	if (blendName == "Opaque") {
		blend = BlendType::Opaque;
	} else if (blendName == "Alpha") {
		blend = BlendType::Alpha;
	} else if (blendName == "AlphaPremultiplied") {
		blend = BlendType::AlphaPremultiplied;
	} else if (blendName == "Add") {
		blend = BlendType::Add;
	} else if (blendName == "Multiply") {
		blend = BlendType::Multiply;
	} else {
		throw Exception("Unknown blend type: " + blendName, HalleyExceptions::Resources);
	}

	cull = fromString<CullingMode>(node["cull"].asString("None"));

	if (node.hasKey("depth")) {
		depthStencil.loadDepth(node["depth"]);
	}
	if (node.hasKey("stencil")) {
		depthStencil.loadStencil(node["stencil"]);
	}
}

void MaterialPass::serialize(Serializer& s) const
{
	s << blend;
	s << shaderAssetId;
	s << depthStencil;
	s << cull;
}

void MaterialPass::deserialize(Deserializer& s)
{
	s >> blend;
	s >> shaderAssetId;
	s >> depthStencil;
	s >> cull;
}

void MaterialPass::createShader(ResourceLoader& loader, String name, const Vector<MaterialAttribute>& attributes)
{
	auto& video = *(loader.getAPI().video);
	auto shaderData = loader.getResources().get<ShaderFile>(shaderAssetId + ":" + video.getShaderLanguage());

	ShaderDefinition definition;
	definition.name = name;
	definition.vertexAttributes = attributes;
	definition.shaders = shaderData->shaders;

	shader = video.createShader(definition);
}
