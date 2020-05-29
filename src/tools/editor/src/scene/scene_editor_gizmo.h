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

	enum class GridSnapMode {
		Disabled,
		Pixel
	};
	
	enum class LineSnapMode {
		Disabled,
		AxisAligned,
		IsometricAxisAligned
	};

	class SceneEditorGizmoHandle {
	public:
		using SnapFunction = std::function<Vector2f(int, Vector2f)>;
		using BoundsCheckFunction = std::function<bool(Vector2f, Vector2f)>;

		SceneEditorGizmoHandle();
		void update(const SceneEditorInputState& inputState, gsl::span<SceneEditorGizmoHandle> handles = {});

		void setId(int id);
		int getId() const;
		
		void setPosition(Vector2f pos, bool snap);
		Vector2f getPosition() const;
		
		bool isOver() const;
		bool isHeld() const;
		bool isSelected() const;

		void setCanDrag(bool enabled);
		void setNotOver();
		void setSelected(bool sel);

		void setBoundsCheck(BoundsCheckFunction boundsCheck);
		void setSnap(SnapFunction snapFunc);
		void setGridSnap(GridSnapMode gridSnap);

	private:
		int id = 0;
		bool over = false;
		bool holding = false;
		bool canDrag = true;
		bool selected = false;

		Vector2f pos;
		Vector2f startOffset;

		BoundsCheckFunction boundsCheck;
		SnapFunction snapFunc;
	};

	class SceneEditorGizmo {
	public:
		struct SnapRules {
			GridSnapMode grid = GridSnapMode::Disabled;
			LineSnapMode line = LineSnapMode::Disabled;
		};

		explicit SceneEditorGizmo(SnapRules snapRules);
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
		SnapRules getSnapRules() const;
		Vector2f solveLineSnap(Vector2f cur, std::optional<Vector2f> prev, std::optional<Vector2f> next) const;

	private:
		std::optional<EntityRef> curEntity;
		ConfigNode* entityData = nullptr;
		SceneEditorOutputState* outputState = nullptr;
		float zoom = 1.0f;

		SnapRules snapRules;

		std::optional<Line> findSnapLine(Vector2f cur, Vector2f ref) const;
	};
}
