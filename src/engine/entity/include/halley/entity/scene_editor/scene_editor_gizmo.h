#pragma once
#include <optional>
#include "entity.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class Painter;

	class SceneEditorGizmo {
	public:
		virtual ~SceneEditorGizmo() = default;

		virtual void update(Time time);
		virtual void draw(const Painter& painter);
		virtual void setSelectedEntity(const std::optional<EntityRef>& entity);
	};
}
