#include "resources/resources.h"
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/api/halley_api.h"
#include "halley/core/graphics/shader.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"
#include <halley/file_formats/text_file.h>
#include "halley/file/byte_serializer.h"

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

void MaterialDefinition::bind(int pass, Painter& painter)
{
	passes[pass].bind(painter);
}

int MaterialDefinition::getNumPasses() const
{
	return int(passes.size());
}

MaterialPass& MaterialDefinition::getPass(int n)
{
	return passes[n];
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

MaterialPass::MaterialPass(BlendType blend, String vertex, String geometry, String pixel)
	: blend(blend)
	, vertex(vertex)
	, geometry(geometry)
	, pixel(pixel)
{
}

void MaterialPass::bind(Painter& painter) const
{
	shader->bind();
	painter.setBlend(getBlend());
}

void MaterialPass::serialize(Serializer& s) const
{
	s << int(blend);
	s << vertex;
	s << geometry;
	s << pixel;
}

void MaterialPass::deserialize(Deserializer& s)
{
	int temp; 
	s >> temp;
	blend = BlendType(temp);
	s >> vertex;
	s >> geometry;
	s >> pixel;	
}

void MaterialPass::createShader(ResourceLoader& loader, String name, const Vector<MaterialAttribute>& attributes)
{
	shader = loader.getAPI().video->createShader(name);
	shader->setAttributes(attributes);
	
	if (vertex != "") {
		shader->addVertexSource(loader.getAPI().getResource<TextFile>("shader/" + vertex)->data);
	}
	if (geometry != "") {
		shader->addGeometrySource(loader.getAPI().getResource<TextFile>("shader/" + geometry)->data);
	}
	if (pixel != "") {
		shader->addPixelSource(loader.getAPI().getResource<TextFile>("shader/" + pixel)->data);
	}
	shader->compile();
}
