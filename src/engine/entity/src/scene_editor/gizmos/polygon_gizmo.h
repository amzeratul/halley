#pragma once
#include "scene_editor/scene_editor_gizmo.h"

namespace Halley {
	class PolygonGizmo final : public SceneEditorGizmo {
	public:
		PolygonGizmo(const String& componentName, const String& fieldName, const ConfigNode& options);
		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;
		std::shared_ptr<UIWidget> makeUI() override;
		
	protected:
		void onEntityChanged() override;
		
	private:
		String componentName;
		String fieldName;
		bool isOpenPolygon;

		VertexList lastStored;
		VertexList vertices;
		std::vector<SceneEditorGizmoHandle> handles;

		VertexList readPoints();
		void writePoints(const VertexList& ps);
		void updateHandles();
		Rect4f getHandleRect(Vector2f pos, float size) const;

		void writePointsIfNeeded();
	};
}
