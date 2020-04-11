#pragma once
#include <optional>
#include "entity.h"
#include "halley/time/halleytime.h"

class Transform2DComponent;

namespace Halley {
	struct SceneEditorInputState;
	class Camera;
	class Painter;

	class SceneEditorGizmo {
	public:
		virtual ~SceneEditorGizmo() = default;

		virtual void update(Time time, const SceneEditorInputState& inputState);
		virtual void draw(Painter& painter) const;
		void setSelectedEntity(const std::optional<EntityRef>& entity);

		void setCamera(const Camera& camera);

	protected:
		virtual void onEntityChanged();

		const std::optional<EntityRef>& getEntity() const;
		std::optional<EntityRef>& getEntity();

		const Transform2DComponent* getTransform() const;
		Transform2DComponent* getTransform();
		
		float getZoom() const;

	private:
		std::optional<EntityRef> curEntity;
		float zoom = 1.0f;
	};
}
