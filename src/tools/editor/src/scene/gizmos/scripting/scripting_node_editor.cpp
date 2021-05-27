#include "scripting_node_editor.h"
#include "scripting_gizmo.h"
using namespace Halley;

ScriptingNodeEditor::ScriptingNodeEditor(ScriptingGizmo& gizmo, UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, uint32_t nodeId, const IScriptNodeType& nodeType, std::optional<Vector2f> pos)
	: UIWidget("scripting_node_editor", {}, UISizer())
	, gizmo(gizmo)
	, factory(factory)
	, entityEditorFactory(entityEditorFactory)
	, nodeId(nodeId)
	, nodeType(nodeType)
	, curSettings(gizmo.getNode(nodeId).getSettings())
{
	if (pos) {
		setAnchor(UIAnchor(Vector2f(), Vector2f(0.0f, 0.5f), pos.value()));
	} else {
		setAnchor(UIAnchor());
	}
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
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "delete", [=] (const UIEvent& event)
	{
		deleteNode();
		destroy();
	});

	setHandle(UIEventType::TextSubmit, [=] (const UIEvent& event)
	{
		applyChanges();
		destroy();
	});

	getWidget("delete")->setEnabled(nodeType.canDelete());

	makeFields(getWidget("nodeFields"));
}

void ScriptingNodeEditor::onAddedToRoot(UIRoot& root)
{
	factory.loadUI(*this, "ui/halley/scripting_node_editor");

	root.registerKeyPressListener(shared_from_this());
}

void ScriptingNodeEditor::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

bool ScriptingNodeEditor::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Esc)) {
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
	const auto nodeId = this->nodeId;
	auto curSettings = ConfigNode(this->curSettings);
	
	Concurrent::execute(gizmo->getExecutionQueue(), [gizmo, nodeId, curSettings = std::move(curSettings)] () {
		gizmo->getNode(nodeId).getSettings() = std::move(curSettings);
		gizmo->saveEntityData();
	});
}

void ScriptingNodeEditor::deleteNode()
{
	auto* gizmo = &this->gizmo;
	const auto nodeId = this->nodeId;

	Concurrent::execute(gizmo->getExecutionQueue(), [gizmo, nodeId] () {
		gizmo->destroyNode(nodeId);
	});
}

void ScriptingNodeEditor::makeFields(const std::shared_ptr<UIWidget>& fieldsRoot)
{
	fieldsRoot->clear();
	
	const auto& types = nodeType.getSettingTypes();

	for (const auto& type: types) {
		const auto params = ComponentFieldParameters(type.name, ComponentDataRetriever(curSettings, type.name), type.defaultValue);
		auto field = entityEditorFactory.makeField(type.type, params, ComponentEditorLabelCreation::Always);
		fieldsRoot->add(field);
	}

	bool foundFocus = false;
	fieldsRoot->descend([&] (const std::shared_ptr<UIWidget>& e)
	{
		if (!foundFocus && e->canReceiveFocus()) {
			foundFocus = true;
			getRoot()->setFocus(e);
		}
	}, false, true);

	if (!foundFocus) {
		getRoot()->setFocus(shared_from_this());
	}
}