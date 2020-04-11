#pragma once
#include "scene_editor_gizmo.h"

namespace Halley {
	class TranslateGizmo final : public SceneEditorGizmo {
	public:
		void update(Time time) override;
		void draw(const Painter& painter) override;
		void setSelectedEntity(const std::optional<EntityRef>& entity) override;
	};
}
