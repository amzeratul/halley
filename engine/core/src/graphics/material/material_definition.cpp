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

static Material* currentMaterial = nullptr;
static int currentPass = 0;

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
	s << int(type);
	s << location;
	s << offset;
}

void MaterialAttribute::deserialize(Deserializer& s)
{
	s >> name;
	int temp;
	s >> temp;
	type = ShaderParameterType(temp);
	s >> location;
	s >> offset;
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

void MaterialDefinition::bind(int pass, Painter& painter, const std::vector<MaterialParameter>& uniforms) const
{
	passes[pass].bind(pass, painter, uniforms);
}

int MaterialDefinition::getNumPasses() const
{
	return int(passes.size());
}

MaterialPass& MaterialDefinition::getPass(int n)
{
	return passes[n];
}

const String& MaterialDefinition::getName() const
{
	return name;
}

std::unique_ptr<MaterialDefinition> MaterialDefinition::loadResource(ResourceLoader& loader)
{
	return std::make_unique<MaterialDefinition>(loader);
}

void MaterialDefinition::serialize(Serializer& s) const
{
	s << name;
	s << passes;
	s << uniforms;
	s << attributes;
	s << vertexStride;
	s << vertexPosOffset;
}

void MaterialDefinition::deserialize(Deserializer& s)
{
	s >> name;
	s >> passes;
	s >> uniforms;
	s >> attributes;
	s >> vertexStride;
	s >> vertexPosOffset;
}

MaterialPass::MaterialPass(BlendType blend, String shaderAssetId)
	: blend(blend)
	, shaderAssetId(shaderAssetId)
{
}

void MaterialPass::bind(int pass, Painter& painter, const std::vector<MaterialParameter>& uniforms) const
{
	painter.setShader(*shader);
	painter.setBlend(getBlend());
	for (auto& u: uniforms) {
		u.bind(pass);
	}
}

void MaterialPass::serialize(Serializer& s) const
{
	s << int(blend);
	s << shaderAssetId;
}

void MaterialPass::deserialize(Deserializer& s)
{
	int temp; 
	s >> temp;
	blend = BlendType(temp);
	s >> shaderAssetId;
}

void MaterialPass::createShader(ResourceLoader& loader, String name, const Vector<MaterialAttribute>& attributes)
{
	ShaderDefinition definition;
	definition.name = name;
	definition.vertexAttributes = attributes;

	auto shaderData = loader.getAPI().getResource<ShaderFile>(shaderAssetId);
	for (auto& shader: shaderData->shaders) {
		definition.shaders[ShaderType(shader.first)] = shader.second;
	}

	shader = loader.getAPI().video->createShader(definition);
}
