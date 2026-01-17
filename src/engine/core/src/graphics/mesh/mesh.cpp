#include "halley/graphics/mesh/mesh.h"
#include "halley/graphics/texture.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_parameter.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resource_data.h"
#include "halley/api/halley_api.h"

using namespace Halley;

Mesh::Mesh(Vector<MeshObject> objects)
	: objects(std::move(objects))
{
}

Mesh::Mesh(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	deserialize(s);

	for (auto& o: objects) {
		o.loadDependencies(loader);
	}
}

std::unique_ptr<Mesh> Mesh::loadResource(ResourceLoader& loader)
{
	return std::make_unique<Mesh>(loader);
}

const Vector<MeshObject>& Mesh::getObjects() const
{
	return objects;
}

void Mesh::serialize(Serializer& s) const
{
	s << objects;
}

void Mesh::deserialize(Deserializer& s)
{
	s >> objects;
}


MeshObject::MeshObject(String name)
	: name(std::move(name))
{
}

void MeshObject::loadDependencies(ResourceLoader& loader)
{
	auto matDef = loader.getResources().get<MaterialDefinition>(materialName);
	material = std::make_unique<Material>(matDef);

	int i = 0;
	for (auto& t: textureNames) {
		auto texture = loader.getResources().get<Texture>(t);
		material->set("tex" + toString(i), texture);
		++i;
	}
}

uint32_t MeshObject::getNumVertices() const
{
	return numVertices;
}

gsl::span<const Byte> MeshObject::getVertexData() const
{
	return gsl::span<const Byte>(vertexData.data(), vertexData.size());
}

gsl::span<const IndexType> MeshObject::getIndices() const
{
	return indices;
}

std::shared_ptr<const Material> MeshObject::getMaterial() const
{
	return material;
}

const String& MeshObject::getName() const
{
	return name;
}

void MeshObject::setVertices(size_t num, Bytes vertexData)
{
	numVertices = uint32_t(num);
	this->vertexData = std::move(vertexData);
}

void MeshObject::setIndices(Vector<IndexType> indices)
{
	this->indices = std::move(indices);
}

void MeshObject::setMaterialName(String name)
{
	this->materialName = std::move(name);
}

void MeshObject::setTextureNames(Vector<String> textureNames)
{
	this->textureNames = std::move(textureNames);
}

void MeshObject::setName(String name)
{
	this->name = std::move(name);
}

void MeshObject::serialize(Serializer& s) const
{
	s << name;
	s << numVertices;
	s << vertexData;
	s << indices;
	s << materialName;
	s << textureNames;
}

void MeshObject::deserialize(Deserializer& s)
{
	s >> name;
	s >> numVertices;
	s >> vertexData;
	s >> indices;
	s >> materialName;
	s >> textureNames;
}
