#pragma once

#include <memory>
#include "halley/maths/quaternion.h"
#include "halley/maths/vector4.h"
#include "halley/maths/vector3.h"
#include "halley/maths/matrix4.h"
#include "halley/time/halleytime.h"

namespace Halley
{
	class Painter;
	class Mesh;
	class Material;

	class MeshRenderer
	{
	public:
		void update(Time t);
		void render(Painter& painter) const;

		std::shared_ptr<const Mesh> getMesh() const;
		Vector3f getPosition() const;
		Vector3f getScale() const;
		Quaternion getRotation() const;

		MeshRenderer& setMesh(std::shared_ptr<const Mesh> mesh);
		MeshRenderer& setPosition(Vector3f pos);
		MeshRenderer& setScale(Vector3f scale);
		MeshRenderer& setRotation(Quaternion rot);

	private:
		Matrix4f matrix;
		Quaternion rot;
		Vector3f pos;
		Vector3f scale;

		std::shared_ptr<const Mesh> mesh;
		std::shared_ptr<Material> material;

		bool dirty = true;
		void updateMatrix();
	};
}
