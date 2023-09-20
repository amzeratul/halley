#include <utility>
#include "halley/graphics/mesh/mesh_renderer.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/mesh/mesh.h"
#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"

using namespace Halley;

void MeshRenderer::update(Time t)
{
	updateMatrix();
}

void MeshRenderer::render(Painter& painter) const
{
	for (const auto& part : mesh->getParts()) {
		if (materialOverride) {
			painter.draw(materialOverride, part.numVertices, part.vertexData.data(), part.indices);
		} else {
			painter.draw(part.material, part.numVertices, part.vertexData.data(), part.indices);
		}
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

MeshRenderer& MeshRenderer::setMesh(std::shared_ptr<const Mesh> mesh)
{
	this->mesh = std::move(mesh);
	dirty = true;
	return *this;
}

MeshRenderer& MeshRenderer::setMaterial(const std::shared_ptr<Material>& material)
{
	this->materialOverride = material->clone();
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

		if (materialOverride) {
			materialOverride->set("u_modelMatrix", matrix);
		} else {
			for (auto& part : mesh->getParts()) {
				part.material->set("u_modelMatrix", matrix);
			}
		}
	}
}
