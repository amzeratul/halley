#pragma once
#include <optional>
#include "halley/core/editor_extensions/scene_editor_interface.h"
#include "halley/entity/entity.h"
#include "halley/time/halleytime.h"

class Transform2DComponent;

namespace Halley {
	struct SceneEditorInputState;
	class Camera;
	class Painter;

	class SceneEditorGizmoHandle {
	public:
		void update(const SceneEditorInputState& inputState, gsl::span<SceneEditorGizmoHandle> handles = {});

		void setBoundsCheck(std::function<bool(Vector2f, Vector2f)> boundsCheck);
		
		void setPosition(Vector2f pos);
		Vector2f getPosition() const;
		
		bool isOver() const;
		bool isHeld() const;
		bool isSelected() const;

		void setCanDrag(bool enabled);
		void setNotOver();
		void setSelected(bool sel);

	private:
		bool over = false;
		bool holding = false;
		bool canDrag = true;
		bool selected = false;

		Vector2f pos;
		Vector2f startOffset;

		std::function<bool(Vector2f, Vector2f)> boundsCheck;
	};

	class SceneEditorGizmo {
	public:
		virtual ~SceneEditorGizmo() = default;

		virtual void update(Time time, const SceneEditorInputState& inputState);
		virtual void draw(Painter& painter) const;
		virtual std::shared_ptr<UIWidget> makeUI();

		void setSelectedEntity(const std::optional<EntityRef>& entity, ConfigNode& entityData);

		void setCamera(const Camera& camera);
		void setOutputState(SceneEditorOutputState& outputState);

		virtual bool isHighlighted() const;
		virtual void deselect();

	protected:
		virtual void onEntityChanged();

		const std::optional<EntityRef>& getEntity() const;
		std::optional<EntityRef>& getEntity();

		const Transform2DComponent* getTransform() const;
		Transform2DComponent* getTransform();

		ConfigNode& getEntityData();
		ConfigNode* getComponentData(const String& name);
		const ConfigNode* getComponentData(const String& name) const;

		void markModified(const String& component, const String& field);
		
		float getZoom() const;

	private:
		std::optional<EntityRef> curEntity;
		ConfigNode* entityData = nullptr;
		SceneEditorOutputState* outputState = nullptr;
		float zoom = 1.0f;
	};
}
