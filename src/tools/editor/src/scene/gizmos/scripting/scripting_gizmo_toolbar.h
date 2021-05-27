#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class ScriptingGizmo;
	class UIFactory;

	class ScriptingGizmoToolbar : public UIWidget {
	public:
		ScriptingGizmoToolbar(UIFactory& factory, ScriptingGizmo& gizmo);

		void onMakeUI() override;
		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

	protected:
		bool onKeyPress(KeyboardKeyPress key) override;
	
	private:
		ScriptingGizmo& gizmo;
		UIFactory& factory;
	};
}
