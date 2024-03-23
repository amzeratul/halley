#include "script_graph_gizmo_ui.h"

#include <utility>

#include "script_graph_editor.h"
#include "halley/scripting/script_graph.h"
#include "src/scene/gizmos/scripting/scripting_base_gizmo.h"
using namespace Halley;

ScriptGizmoUI::ScriptGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, ScriptGraphEditor& graphEditor)
	: GraphGizmoUI(std::move(keyboard), std::move(clipboard), graphEditor, std::make_unique<ScriptingBaseGizmo>(factory, entityEditorFactory, resources, scriptNodeTypes))
{
	scriptGizmo = dynamic_cast<ScriptingBaseGizmo*>(gizmo.get());

	setHandle(UIEventType::CanvasDoubleClicked, [=] (const UIEvent& event)
	{
		if (auto underMouse = gizmo->getNodeUnderMouse()) {
			onDoubleClick(underMouse->nodeId);
		}
	});
}

void ScriptGizmoUI::load(BaseGraph& graph)
{
	scriptGizmo->setGraph(dynamic_cast<ScriptGraph*>(&graph));
	updateNodes();
}

void ScriptGizmoUI::setState(ScriptState* state)
{
	scriptGizmo->setState(state);
}

void ScriptGizmoUI::setCurNodeDevConData(const String& str)
{
	scriptGizmo->setCurNodeDevConData(str);
}

void ScriptGizmoUI::setDebugDisplayData(HashMap<int, String> values)
{
	scriptGizmo->setDebugDisplayData(std::move(values));
}

void ScriptGizmoUI::updateNodes()
{
	scriptGizmo->updateNodes();
}

void ScriptGizmoUI::setEntityTargets(Vector<String> entityTargets)
{
	scriptGizmo->setEntityTargets(std::move(entityTargets));
}

void ScriptGizmoUI::onDoubleClick(GraphNodeId nodeId)
{
	auto open = [&] (const String& scriptId)
	{
		if (!scriptId.isEmpty()) {
			auto uri = "asset:scriptGraph:" + scriptId;
			Logger::logDev("Navigating to " + uri);
			sendEvent(UIEvent(UIEventType::NavigateToAsset, getId(), std::move(uri)));
		}
	};

	const auto& node = scriptGizmo->getGraph().getNodes()[nodeId];
	if (node.getType() == "callExternal") {
		open(node.getSettings()["function"].asString(""));
	} else if (node.getType() == "startScript") {
		open(node.getSettings()["script"].asString(""));
	} else if (node.getType() == "sendMessage") {
		const auto msgType = ScriptMessageType(node.getSettings()["message"]);
		open(msgType.script);
	}
}
