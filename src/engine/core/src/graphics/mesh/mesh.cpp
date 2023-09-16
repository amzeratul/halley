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
	//s << materialName; // TODO handle materials correctly
	//s << textureNames;
}

void MeshPart::deserialize(Deserializer& d)
{
	d >> numVertices;
	d >> vertexData;
	d >> indices;
	//d >> materialName;
	//d >> textureNames;
}

Mesh::Mesh()
{
}

Mesh::Mesh(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	deserialize(s);

	auto matDef = loader.getResources().get<MaterialDefinition>(materialName);
	material = std::make_unique<Material>(matDef);

	int i = 0;
	for (auto& t: textureNames) {
		auto texture = loader.getResources().get<Texture>(t);
		material->set("tex" + toString(i), texture);
		++i;
	}
}

std::unique_ptr<Mesh> Mesh::loadResource(ResourceLoader& loader)
{
	return std::make_unique<Mesh>(loader);
}

std::shared_ptr<const Material> Mesh::getMaterial() const
{
	return material;
}

void Mesh::setMaterialName(String name)
{
	this->materialName = std::move(name);
}

void Mesh::setTextureNames(Vector<String> textureNames)
{
	this->textureNames = std::move(textureNames);
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
	s << static_cast<uint32_t>(meshParts.size());
	s << materialName;
	s << textureNames;

	for (const auto& part : meshParts) {
		part.serialize(s);
	}
}

void Mesh::deserialize(Deserializer& d)
{
    d >> count;
	d >> materialName;
	d >> textureNames;

	for (uint32_t i = 0; i < count; ++i) {
		MeshPart part{};
		part.deserialize(d);
		addPart(std::move(part));
	}
}
