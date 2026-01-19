#include <utility>
#include "halley/graphics/mesh/mesh_renderer.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/mesh/mesh.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"

using namespace Halley;

MeshRenderer::MeshRenderer(std::shared_ptr<const Mesh> mesh)
{
	resetTransform();
	setMesh(std::move(mesh));
}

void MeshRenderer::update(Time t)
{
	updateMatrix();
}

void MeshRenderer::render(Painter& painter) const
{
	// TODO: multi-thread support?

	const size_t n = mesh->getObjects().size();
	for (size_t i = 0; i < n; ++i) {
		const auto& object = mesh->getObjects()[i];
		painter.draw(materials[i], object.getNumVertices(), object.getVertexData().data(), object.getIndices());
	}
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

void MeshRenderer::resetTransform()
{
	pos = {};
	scale = Vector3f(1, 1, 1);
	rot = Quaternion();
}

MeshRenderer& MeshRenderer::setMesh(std::shared_ptr<const Mesh> mesh)
{
	const size_t n = mesh->getObjects().size();
	materials.resize(n);
	for (size_t i = 0; i < n; ++i) {
		materials[i] = mesh->getObjects()[i].getMaterial()->clone();
	}

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

		for (auto& material : materials) {
			material->set("u_modelMatrix", matrix);
		}
	}
}
