#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class ScriptingBaseGizmo;
	class UIFactory;

	class ScriptingGizmoToolbar : public UIWidget {
	public:
		ScriptingGizmoToolbar(UIFactory& factory, ScriptingBaseGizmo& gizmo);

		void onMakeUI() override;
		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;
	
	private:
		ScriptingBaseGizmo& gizmo;
		UIFactory& factory;
	};
}
