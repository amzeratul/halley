#include "script_gizmo_ui.h"
using namespace Halley;

ScriptGizmoUI::ScriptGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, ModifiedCallback modifiedCallback)
	: UIWidget("scriptGizmoUI", {}, {})
	, factory(factory)
	, resources(resources)
	, keyboard(std::move(keyboard))
	, clipboard(std::move(clipboard))
	, gizmo(factory, entityEditorFactory, nullptr, resources, scriptNodeTypes)
	, modifiedCallback(std::move(modifiedCallback))
{
	gizmo.setModifiedCallback([=] ()
	{
		onModified();
	});
	gizmo.setEventSink(*this);

	setHandle(UIEventType::MouseWheel, [=] (const UIEvent& event)
	{
		if (inputState.mousePos) {
			gizmo.onMouseWheel(*inputState.mousePos, event.getIntData(), event.getKeyMods());
		}
	});

	setHandle(UIEventType::CanvasDoubleClicked, [=] (const UIEvent& event)
	{
		if (auto underMouse = gizmo.getNodeUnderMouse()) {
			onDoubleClick(underMouse->nodeId);
		}
	});
}

void ScriptGizmoUI::onAddedToRoot(UIRoot& root)
{
	gizmo.setUIRoot(root);
	root.registerKeyPressListener(shared_from_this());
}

void ScriptGizmoUI::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

void ScriptGizmoUI::load(ScriptGraph& graph)
{
	gizmo.setGraph(&graph);
	updateNodes();
}

void ScriptGizmoUI::setState(ScriptState* state)
{
	gizmo.setState(state);
}

void ScriptGizmoUI::update(Time time, bool moved)
{
	inputState.altHeld = keyboard->isButtonDown(KeyCode::LAlt) || keyboard->isButtonDown(KeyCode::RAlt);
	inputState.ctrlHeld = keyboard->isButtonDown(KeyCode::LCtrl) || keyboard->isButtonDown(KeyCode::RCtrl);
	inputState.shiftHeld = keyboard->isButtonDown(KeyCode::LShift) || keyboard->isButtonDown(KeyCode::RShift);
	inputState.spaceHeld = keyboard->isButtonDown(KeyCode::Space);

	gizmo.setBasePosition(getPosition());
	if (time > 0.00001) {
		inputState.rawMousePos = inputState.mousePos;
		gizmo.update(time, inputState);
		inputState.clear();
	}
}

void ScriptGizmoUI::draw(UIPainter& painter) const
{
	painter.draw([=] (Painter& p)
	{
		gizmo.draw(p);

		if (inputState.selectionBox) {
			const float zoom = 1.0f;
			const auto rect = inputState.selectionBox.value() + Vector2f(0.5f, 0.5f) / zoom;
			p.drawRect(rect, 3.0f / zoom, Colour4f(0, 0, 0, 0.5f));
			p.drawRect(rect, 1.0f / zoom, Colour4f(1, 1, 1));
		}
	});
}

void ScriptGizmoUI::setZoom(float zoom)
{
	gizmo.setZoom(zoom);
}

void ScriptGizmoUI::setAutoConnectPins(bool autoConnect)
{
	gizmo.setAutoConnectPins(autoConnect);
}

bool ScriptGizmoUI::isHighlighted() const
{
	return gizmo.isHighlighted();
}

std::shared_ptr<UIWidget> ScriptGizmoUI::makeUI()
{
	return gizmo.makeUI();
}

std::optional<BaseGraphRenderer::NodeUnderMouseInfo> ScriptGizmoUI::getNodeUnderMouse() const
{
	return gizmo.getNodeUnderMouse();
}

void ScriptGizmoUI::setCurNodeDevConData(const String& str)
{
	gizmo.setCurNodeDevConData(str);
}

void ScriptGizmoUI::setDebugDisplayData(HashMap<int, String> values)
{
	gizmo.setDebugDisplayData(std::move(values));
}

void ScriptGizmoUI::updateNodes()
{
	gizmo.updateNodes();
}

void ScriptGizmoUI::setEntityTargets(Vector<String> entityTargets)
{
	gizmo.setEntityTargets(std::move(entityTargets));
}

void ScriptGizmoUI::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	focus();

	inputState.mousePos = mousePos;
	updateSelectionBox();
	if (button == 0) {
		inputState.leftClickPressed = true;
		inputState.leftClickHeld = true;
		if (!isHighlighted()) {
			dragStart = mousePos;
		}
	} else if (button == 1) {
		inputState.middleClickPressed = true;
		inputState.middleClickHeld = true;
	} else if (button == 2) {
		inputState.rightClickPressed = true;
		inputState.rightClickHeld = true;
		if (!isHighlighted()) {
			gizmo.addNode();
		}
	}
}

void ScriptGizmoUI::releaseMouse(Vector2f mousePos, int button)
{
	inputState.mousePos = mousePos;
	updateSelectionBox();
	if (button == 0) {
		inputState.leftClickReleased = true;
		inputState.leftClickHeld = false;
		dragStart = {};
	} else if (button == 1) {
		inputState.middleClickReleased = true;
		inputState.middleClickHeld = false;
	} else if (button == 2) {
		inputState.rightClickReleased = true;
		inputState.rightClickHeld = false;
	}
}

void ScriptGizmoUI::onMouseOver(Vector2f mousePos)
{
	inputState.mousePos = mousePos;
	updateSelectionBox();
}

void ScriptGizmoUI::onMouseLeft(Vector2f mousePos)
{
	inputState.mousePos = std::nullopt;
}

bool ScriptGizmoUI::ignoreClip() const
{
	return true;
}

bool ScriptGizmoUI::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::A, KeyMods::Ctrl)) {
		gizmo.addNode();
		return true;
	}

	if (key.is(KeyCode::Delete)) {
		gizmo.deleteSelection();
		return true;
	}

	if (key.is(KeyCode::C, KeyMods::Ctrl)) {
		gizmo.copySelectionToClipboard(clipboard);
		return true;
	}

	if (key.is(KeyCode::X, KeyMods::Ctrl)) {
		gizmo.cutSelectionToClipboard(clipboard);
		return true;
	}

	if (key.is(KeyCode::V, KeyMods::Ctrl)) {
		gizmo.pasteFromClipboard(clipboard);
		return true;
	}

	if (key.is(KeyCode::D, KeyMods::Ctrl)) {
		gizmo.paste(gizmo.copySelection());
		return true;
	}

	return false;
}

bool ScriptGizmoUI::canReceiveFocus() const
{
	return true;
}

void ScriptGizmoUI::onModified()
{
	if (modifiedCallback) {
		modifiedCallback();
	}
}

void ScriptGizmoUI::updateSelectionBox()
{
	inputState.selectionBox.reset();
	if (dragStart && inputState.mousePos) {
		const auto rect = Rect4f(*dragStart, *inputState.mousePos);
		if (rect.getWidth() >= 2 || rect.getHeight() >= 2) {
			inputState.selectionBox = rect;
		}
	}
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

	const auto& node = gizmo.getGraph().getNodes()[nodeId];
	if (node.getType() == "callExternal") {
		open(node.getSettings()["function"].asString(""));
	} else if (node.getType() == "startScript") {
		open(node.getSettings()["script"].asString(""));
	} else if (node.getType() == "sendMessage") {
		const auto msgType = ScriptMessageType(node.getSettings()["message"]);
		open(msgType.script);
	}
}
