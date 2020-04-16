#pragma once
#include "scene_editor/scene_editor_gizmo.h"

namespace Halley {
	class PolygonGizmo final : public SceneEditorGizmo {
	public:
		PolygonGizmo(const String& componentName, const String& fieldName, const ConfigNode& options);
		void update(Time time, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter) const override;

	private:
		String componentName;
		String fieldName;
		bool isOpenPolygon;
	};
}
