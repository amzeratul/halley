#pragma once

#include "graph_gizmo_ui.h"
#include "src/scene/gizmos/scripting/scripting_base_gizmo.h"

namespace Halley {
	class ScriptGraphEditor;

	class ScriptGizmoUI : public GraphGizmoUI {
	public:
		ScriptGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, ScriptGraphEditor& graphEditor);
		
		void load(ScriptGraph& graph);
		void setState(ScriptState* state);
		
		void setCurNodeDevConData(const String& str);
		void setDebugDisplayData(HashMap<int, String> values);

		void updateNodes();

		void setEntityTargets(Vector<String> entityTargets);

	private:
		ScriptingBaseGizmo* scriptGizmo = nullptr;

		void onDoubleClick(GraphNodeId nodeId);
	};
}
