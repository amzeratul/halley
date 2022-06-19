#include "scripting_node_editor.h"
#include "scripting_gizmo.h"
using namespace Halley;

ScriptingNodeEditor::ScriptingNodeEditor(ScriptingBaseGizmo& gizmo, UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, std::optional<uint32_t> nodeId, const IScriptNodeType& nodeType, std::optional<Vector2f> pos)
	: UIWidget("scripting_node_editor", {}, UISizer())
	, gizmo(gizmo)
	, factory(factory)
	, entityEditorFactory(entityEditorFactory)
	, nodeId(nodeId)
	, nodeType(nodeType)
	, curSettings(nodeId ? ConfigNode(gizmo.getNode(*nodeId).getSettings()) : ConfigNode::MapType())
{
	UIAnchor anchor;
	if (pos) {
		anchor = UIAnchor(Vector2f(), Vector2f(0.0f, 0.5f), pos.value());
	}
	anchor.setAutoBounds(true);
	setAnchor(anchor);

	setModal(true);
	factory.loadUI(*this, "halley/scripting_node_editor");
}

void ScriptingNodeEditor::onMakeUI()
{
	getWidgetAs<UILabel>("name")->setText(LocalisedString::fromUserString(nodeType.getName()));

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		applyChanges();
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		cancelChanges();
		destroy();
	});

	setHandle(UIEventType::TextSubmit, [=] (const UIEvent& event)
	{
		applyChanges();
		destroy();
	});
	
	makeFields(getWidget("nodeFields"));
}

void ScriptingNodeEditor::onAddedToRoot(UIRoot& root)
{
	root.registerKeyPressListener(shared_from_this());
	root.focusNext(false);
}

void ScriptingNodeEditor::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

bool ScriptingNodeEditor::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Esc)) {
		cancelChanges();
		destroy();
		return true;
	}

	if (key.is(KeyCode::Enter)) {
		applyChanges();
		destroy();
		return true;
	}

	return false;
}

void ScriptingNodeEditor::applyChanges()
{
	auto* gizmo = &this->gizmo;
	const auto type = nodeType.getId();
	
	Concurrent::execute(gizmo->getExecutionQueue(), [type, gizmo, nodeId = this->nodeId, curSettings = ConfigNode(curSettings)] () mutable {
		if (!nodeId) {
			nodeId = gizmo->addNode(type, Vector2f(), std::move(curSettings));
		} else {
			gizmo->getNode(*nodeId).getSettings() = std::move(curSettings);
		}
		gizmo->getGraph().validateNodePins(*nodeId);
		gizmo->onModified();
	});
}

void ScriptingNodeEditor::cancelChanges()
{
}

void ScriptingNodeEditor::deleteNode()
{
	auto* gizmo = &this->gizmo;
	const auto nodeId = *this->nodeId;

	Concurrent::execute(gizmo->getExecutionQueue(), [gizmo, nodeId] () {
		gizmo->destroyNode(nodeId);
	});
}

void ScriptingNodeEditor::makeFields(const std::shared_ptr<UIWidget>& fieldsRoot)
{
	fieldsRoot->clear();
	
	const auto& types = nodeType.getSettingTypes();

	for (const auto& type: types) {
		const auto params = ComponentFieldParameters(type.name, ComponentDataRetriever(curSettings, type.name, type.name), type.defaultValue);
		auto field = entityEditorFactory.makeField(type.type, params, ComponentEditorLabelCreation::Always);
		fieldsRoot->add(field);
	}
}