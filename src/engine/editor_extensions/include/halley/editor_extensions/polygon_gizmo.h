#pragma once
#include "scene_editor_gizmo.h"
#include "halley/maths/polygon.h"
#include "halley/ui/widgets/ui_button.h"

namespace Halley {
	class UIList;

	enum class PolygonGizmoMode {
		Move,
		Append,
		Insert,
		Delete
	};

	template <>
	struct EnumNames<PolygonGizmoMode> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
					"move",
					"append",
					"insert",
					"delete"
				}};
		}
	};

	class PolygonGizmo : public SceneEditorGizmo {
	public:
		PolygonGizmo(SnapRules snapRules, String componentName, String fieldName, bool isOpenPolygon, Colour4f colour, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow);
		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter, const ISceneEditor& sceneEditor) const override;
		std::shared_ptr<UIWidget> makeUI() override;

		bool isHighlighted() const override;
		void deselect() override;

		Vector<String> getHighlightedComponents() const override;
		
	protected:
		void onEntityChanged() override;
		void refreshEntity() override;

		UIFactory& factory;
		String componentName;
		String fieldName;
		bool isOpenPolygon;
		Colour4f colour;
		ISceneEditorWindow& sceneEditorWindow;
	
		std::optional<VertexList> lastStored;
		std::optional<VertexList> vertices;
		mutable VertexList worldSpaceVertices;

	private:
		std::optional<Vertex> preview;
		size_t previewIndex = 0;
		Vector<SceneEditorGizmoHandle> handles;

		PolygonGizmoMode mode = PolygonGizmoMode::Move;
		std::optional<PolygonGizmoMode> pendingMode;
		std::shared_ptr<UIWidget> uiRoot;
		std::shared_ptr<UIList> uiMode;
		std::shared_ptr<UIButton> uiAddComponent;

		bool enableLineSnap = false;
		int highlightCooldown = 0;

		void loadEntity(PolygonGizmoMode mode);
		std::optional<VertexList> readPoints();
		void writePoints(const VertexList& ps);
		void writePointsIfNeeded();
		ConfigNode& getField(ConfigNode& node, const String& fieldName);

		void loadHandlesFromVertices();
		void setHandleIndices();
		Rect4f getHandleRect(Vector2f pos, float size) const;
		SceneEditorGizmoHandle makeHandle(Vector2f pos) const;
		int updateHandles(const SceneEditorInputState& inputState);

		void setMode(PolygonGizmoMode mode);
		void requestSetMode(PolygonGizmoMode mode);
		void updateUI();

		std::pair<Vertex, size_t> findInsertPoint(Vector2f pos) const;

		Vector2f localToWorld(Vector2f localPos) const;
		Vector2f worldToLocal(Vector2f worldPos) const;

		Vector2f snapVertex(int id, Vector2f pos) const;
		const SceneEditorGizmoHandle* tryGetHandle(int id) const;
	};
}
