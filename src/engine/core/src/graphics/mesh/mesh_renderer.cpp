#include <utility>
#include "halley/core/graphics/mesh/mesh_renderer.h"
#include "halley/core/graphics/painter.h"
#include "halley/core/graphics/mesh/mesh.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material_parameter.h"

using namespace Halley;

void MeshRenderer::update(Time t)
{
	updateMatrix();
}

void MeshRenderer::render(Painter& painter) const
{
	painter.draw(material, mesh->getNumVertices(), mesh->getVertexData().data(), mesh->getIndices());
}

std::shared_ptr<const Mesh> MeshRenderer::getMesh() const
{
	return mesh;
}

Vector3f MeshRenderer::getPosition() const
{
	return pos;
}

Vector3f MeshRenderer::getScale() const
{
	return scale;
}

Quaternion MeshRenderer::getRotation() const
{
	return rot;
}

MeshRenderer& MeshRenderer::setMesh(std::shared_ptr<const Mesh> mesh)
{
	material = mesh->getMaterial()->clone();
	this->mesh = std::move(mesh);
	dirty = true;
	return *this;
}

MeshRenderer& MeshRenderer::setPosition(Vector3f pos)
{
	this->pos = pos;
	dirty = true;
	return *this;
}

MeshRenderer& MeshRenderer::setScale(Vector3f scale)
{
	this->scale = scale;
	dirty = true;
	return *this;
}

MeshRenderer& MeshRenderer::setRotation(Quaternion rot)
{
	this->rot = rot;
	dirty = true;
	return *this;
}

void MeshRenderer::updateMatrix()
{
	if (dirty) {
		matrix.loadIdentity();
		matrix.translate(pos);
		matrix.rotate(rot);
		matrix.scale(scale);
		dirty = false;
		material->set("u_modelMatrix", matrix);
	}
}
