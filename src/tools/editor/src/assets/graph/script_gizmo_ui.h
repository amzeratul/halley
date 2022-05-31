#pragma once

#include "halley/ui/ui_widget.h"
#include "src/scene/gizmos/scripting/scripting_base_gizmo.h"

namespace Halley {
	class ScriptGizmoUI : public UIWidget {
	public:
		ScriptGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes);

		void onAddedToRoot(UIRoot& root) override;

		void load(ScriptGraph& graph);

		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

	private:
		UIFactory& factory;
		Resources& resources;
		ScriptingBaseGizmo gizmo;

		void onModified();
	};
}
