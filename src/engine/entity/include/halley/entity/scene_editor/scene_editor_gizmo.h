#pragma once
#include <optional>
#include "entity.h"
#include "halley/time/halleytime.h"

class Transform2DComponent;

namespace Halley {
	struct SceneEditorInputState;
	class Camera;
	class Painter;

	class SceneEditorGizmoHandle {
	public:
		void update(const SceneEditorInputState& inputState);

		void setBoundsCheck(std::function<bool(Vector2f, Vector2f)> boundsCheck);
		
		void setPosition(Vector2f pos);
		Vector2f getPosition() const;
		
		bool isOver() const;
		bool isHeld() const;

	private:
		bool over = false;
		bool holding = false;

		Vector2f pos;
		Vector2f startOffset;

		std::function<bool(Vector2f, Vector2f)> boundsCheck;
	};

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
