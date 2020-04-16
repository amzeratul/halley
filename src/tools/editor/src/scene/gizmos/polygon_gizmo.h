#pragma once
#include "../scene_editor_gizmo.h"

namespace Halley {
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
		PolygonGizmo(const String& componentName, const String& fieldName, const ConfigNode& options, UIFactory& factory);
		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;
		std::shared_ptr<UIWidget> makeUI() override;
		
	protected:
		void onEntityChanged() override;
		
	private:
		UIFactory& factory;
		String componentName;
		String fieldName;
		bool isOpenPolygon;

		VertexList lastStored;
		VertexList vertices;
		Vertex preview;
		std::vector<SceneEditorGizmoHandle> handles;

		PolygonGizmoMode mode = PolygonGizmoMode::Move;
		std::shared_ptr<UIList> uiList;

		VertexList readPoints();
		void writePoints(const VertexList& ps);
		void updateHandles();
		Rect4f getHandleRect(Vector2f pos, float size) const;

		void writePointsIfNeeded();
		void setMode(PolygonGizmoMode mode);

		SceneEditorGizmoHandle makeHandle(Vector2f pos);
	};
}
