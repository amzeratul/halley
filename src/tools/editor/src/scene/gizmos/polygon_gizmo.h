#pragma once
#include "../scene_editor_gizmo.h"

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

	class PolygonGizmo final : public SceneEditorGizmo {
	public:
		PolygonGizmo(SnapRules snapRules, String componentName, String fieldName, const ConfigNode& options, UIFactory& factory);
		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;
		std::shared_ptr<UIWidget> makeUI() override;

		bool isHighlighted() const override;
		void deselect() override;
		
	protected:
		void onEntityChanged() override;

	private:
		UIFactory& factory;
		String componentName;
		String fieldName;
		bool isOpenPolygon;
		Colour4f colour;

		VertexList lastStored;
		VertexList vertices;
		mutable VertexList worldSpaceVertices;
		std::optional<Vertex> preview;
		size_t previewIndex = 0;
		std::vector<SceneEditorGizmoHandle> handles;

		PolygonGizmoMode mode = PolygonGizmoMode::Move;
		std::shared_ptr<UIList> uiList;

		bool enableLineSnap = false;
		int highlightCooldown = 0;

		VertexList readPoints();
		void writePoints(const VertexList& ps);
		void writePointsIfNeeded();
		ConfigNode& getField(ConfigNode& node, const String& fieldName);

		void loadHandlesFromVertices();
		void setHandleIndices();
		Rect4f getHandleRect(Vector2f pos, float size) const;
		SceneEditorGizmoHandle makeHandle(Vector2f pos) const;
		int updateHandles(const SceneEditorInputState& inputState);

		void setMode(PolygonGizmoMode mode);

		std::pair<Vertex, size_t> findInsertPoint(Vector2f pos) const;

		Vector2f localToWorld(Vector2f localPos) const;
		Vector2f worldToLocal(Vector2f worldPos) const;

		Vector2f snapVertex(int id, Vector2f pos) const;
		const SceneEditorGizmoHandle* tryGetHandle(int id) const;
	};
}
