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
	loadDependencies(loader.getResources());
}

void Mesh::loadDependencies(Resources& resources)
{
	for (auto& o: objects) {
		o.loadDependencies(resources);
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

std::pair<Vector3f, Vector3f> Mesh::getBounds() const
{
	if (objects.empty()) {
		return {};
	} else {
		auto [p0, p1] = objects[0].getBounds();
		for (size_t i = 1; i < objects.size(); ++i) {
			auto [b0, b1] = objects[i].getBounds();
			p0 = Vector3f::min(p0, b0);
			p1 = Vector3f::max(p1, b1);
		}
		return { p0, p1 };
	}
}

std::pair<Vector3f, Vector3f> Mesh::getCentreAndSize() const
{
	const auto [p0, p1] = getBounds();
	return { (p0 + p1) * 0.5f, p1 - p0 };
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

void MeshObject::loadDependencies(Resources& resources)
{
	auto matDef = resources.get<MaterialDefinition>(materialName);
	material = matDef->createMaterial();

	int i = 0;
	for (auto& t: textureNames) {
		auto texture = resources.get<Texture>(t);
		material->set("tex" + toString(i), texture);
		++i;
	}

	for (const auto& [paramId, param]: materialParams.asMap()) {
		if (material->hasParameter(paramId)) {
			material->set(paramId, param);
		}
	}
}

size_t MeshObject::getNumVertices() const
{
	return vertexData.size();
}

gsl::span<const MeshObject::VertexData> MeshObject::getVertexData() const
{
	return vertexData.const_span();
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

void MeshObject::setVertices(Vector<VertexData> vertexData)
{
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

void MeshObject::setMaterialParams(ConfigNode params)
{
	this->materialParams = std::move(params);
}

std::pair<Vector3f, Vector3f> MeshObject::getBounds() const
{
	if (vertexData.empty()) {
		return {};
	}

	Vector3f a = vertexData[0].pos.xyz();
	Vector3f b = vertexData[0].pos.xyz();

	for (const auto& v: vertexData) {
		a = Vector3f::min(a, v.pos.xyz());
		b = Vector3f::max(b, v.pos.xyz());
	}

	return { a, b };
}

void MeshObject::serialize(Serializer& s) const
{
	s << name;
	s << vertexData;
	s << indices;
	s << materialName;
	s << textureNames;
	s << materialParams;
}

void MeshObject::deserialize(Deserializer& s)
{
	s >> name;
	s >> vertexData;
	s >> indices;
	s >> materialName;
	s >> textureNames;
	s >> materialParams;
}


void MeshObject::VertexData::serialize(Serializer& s) const
{
	s << pos;
	s << normal;
	s << colour;
	s << texCoord0;
}

void MeshObject::VertexData::deserialize(Deserializer& s)
{
	s >> pos;
	s >> normal;
	s >> colour;
	s >> texCoord0;
}
