#include "resources/resources.h"
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/shader.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/file/byte_serializer.h"
#include "halley/text/string_converter.h"
#include "halley/file_formats/binary_file.h"

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
		default: throw Exception("Unknown type: " + toString(int(type)));
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
}

MaterialPass::MaterialPass()
	: blend(BlendType::Undefined)
{}

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

MaterialPass::MaterialPass(BlendType blend, String shaderAssetId)
	: blend(blend)
	, shaderAssetId(shaderAssetId)
{
}

void MaterialPass::serialize(Serializer& s) const
{
	s << blend;
	s << shaderAssetId;
}

void MaterialPass::deserialize(Deserializer& s)
{
	s >> blend;
	s >> shaderAssetId;
}

void MaterialPass::createShader(ResourceLoader& loader, String name, const Vector<MaterialAttribute>& attributes)
{
	auto& api = loader.getAPI();
	auto& video = *api.video;
	auto shaderData = api.getResource<ShaderFile>(shaderAssetId + ":" + video.getShaderLanguage());

	ShaderDefinition definition;
	definition.name = name;
	definition.vertexAttributes = attributes;
	definition.shaders = shaderData->shaders;

	shader = video.createShader(definition);
}
