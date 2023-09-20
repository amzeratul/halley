#include "halley/graphics/mesh/mesh.h"
#include "halley/graphics/texture.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_parameter.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resource_data.h"
#include "halley/api/halley_api.h"

using namespace Halley;

void MeshPart::serialize(Serializer& s) const
{
	s << numVertices;
	s << vertexData;
	s << indices;
	s << materialName;
	s << textureNames;
}

void MeshPart::deserialize(Deserializer& d, ResourceLoader& loader)
{
	d >> numVertices;
	d >> vertexData;
	d >> indices;
	d >> materialName;
	d >> textureNames;

	auto matDef = loader.getResources().get<MaterialDefinition>(materialName);
    material = std::make_unique<Material>(matDef);
	
	int i = 0;
	for (auto& t: textureNames) {
		auto texture = loader.getResources().get<Texture>(t);
		material->set("tex" + toString(i), texture);
		++i;
	}
}

Mesh::Mesh()
{
}

Mesh::Mesh(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	deserialize(s, loader);
}

std::unique_ptr<Mesh> Mesh::loadResource(ResourceLoader& loader)
{
	return std::make_unique<Mesh>(loader);
}

void Mesh::addPart(MeshPart part)
{
	meshParts.emplace_back(std::move(part));
}

const Vector<MeshPart>& Mesh::getParts() const
{
	return meshParts;
}

void Mesh::serialize(Serializer& s) const
{
	s << static_cast<uint32_t>(meshParts.size()); // TODO can this be done more elegant?

	for (const auto& part : meshParts) {
		part.serialize(s);
	}
}

void Mesh::deserialize(Deserializer& d, ResourceLoader& loader)
{
    d >> count; // TODO can this be done more elegant?

	for (uint32_t i = 0; i < count; ++i) {
		MeshPart part{};
		part.deserialize(d, loader);
		addPart(std::move(part));
	}
}
