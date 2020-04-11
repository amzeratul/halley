#pragma once
#include <optional>
#include "entity.h"
#include "halley/time/halleytime.h"

class Transform2DComponent;

namespace Halley {
	class Painter;

	class SceneEditorGizmo {
	public:
		virtual ~SceneEditorGizmo() = default;

		virtual void update(Time time);
		virtual void draw(Painter& painter) const;
		void setSelectedEntity(const std::optional<EntityRef>& entity);

	protected:
		virtual void onEntityChanged();
		const Transform2DComponent* getTransform() const;
		Transform2DComponent* getTransform();

	private:
		std::optional<EntityRef> curEntity;
	};
}
