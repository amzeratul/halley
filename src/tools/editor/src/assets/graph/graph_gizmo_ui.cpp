#include "graph_gizmo_ui.h"
#include "graph_editor.h"

using namespace Halley;

GraphGizmoUI::GraphGizmoUI(std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, IGraphEditor& graphEditor, std::unique_ptr<BaseGraphGizmo> baseGizmo)
	: UIWidget("graphGizmoUI", {}, {})
	, gizmo(std::move(baseGizmo))
	, keyboard(std::move(keyboard))
	, clipboard(std::move(clipboard))
	, graphEditor(graphEditor)
{
	gizmo->setModifiedCallback([=] ()
	{
		onModified();
	});
	gizmo->setEventSink(*this);

	setHandle(UIEventType::MouseWheel, [=] (const UIEvent& event)
	{
		if (inputState.mousePos) {
			gizmo->onMouseWheel(*inputState.mousePos, event.getIntData(), event.getKeyMods());
		}
	});
}

void GraphGizmoUI::onAddedToRoot(UIRoot& root)
{
	gizmo->setUIRoot(root);
	root.registerKeyPressListener(shared_from_this());
}

void GraphGizmoUI::onRemovedFromRoot(UIRoot& root)
{
	root.removeKeyPressListener(*this);
}

void GraphGizmoUI::update(Time time, bool moved)
{
	inputState.altHeld = keyboard->isButtonDown(KeyCode::LAlt) || keyboard->isButtonDown(KeyCode::RAlt);
	inputState.ctrlHeld = keyboard->isButtonDown(KeyCode::LCtrl) || keyboard->isButtonDown(KeyCode::RCtrl);
	inputState.shiftHeld = keyboard->isButtonDown(KeyCode::LShift) || keyboard->isButtonDown(KeyCode::RShift);
	inputState.spaceHeld = keyboard->isButtonDown(KeyCode::Space);

	gizmo->setBasePosition(getPosition());
	if (time > 0.00001) {
		inputState.rawMousePos = inputState.mousePos;
		gizmo->update(time, inputState);
		inputState.clear();
	}
}

void GraphGizmoUI::draw(UIPainter& painter) const
{
	painter.draw([=] (Painter& p)
	{
		gizmo->draw(p);

		if (inputState.selectionBox) {
			const auto rect = inputState.selectionBox.value();
			p.drawRect(rect, 3.0f, Colour4f(0, 0, 0, 0.5f));
			p.drawRect(rect, 1.0f, Colour4f(1, 1, 1));
		}
	});
}

void GraphGizmoUI::setZoom(float zoom)
{
	gizmo->setZoom(zoom);
}

void GraphGizmoUI::setAutoConnectPins(bool autoConnect)
{
	gizmo->setAutoConnectPins(autoConnect);
}

bool GraphGizmoUI::isHighlighted() const
{
	return gizmo->isHighlighted();
}

std::shared_ptr<UIWidget> GraphGizmoUI::makeUI()
{
	return gizmo->makeUI();
}

std::optional<BaseGraphRenderer::NodeUnderMouseInfo> GraphGizmoUI::getNodeUnderMouse() const
{
	return gizmo->getNodeUnderMouse();
}

void GraphGizmoUI::load(BaseGraph& graph)
{
}

void GraphGizmoUI::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
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
			gizmo->addNode();
		}
	}
}

void GraphGizmoUI::releaseMouse(Vector2f mousePos, int button)
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

void GraphGizmoUI::onMouseOver(Vector2f mousePos)
{
	inputState.mousePos = mousePos;
	updateSelectionBox();
}

void GraphGizmoUI::onMouseLeft(Vector2f mousePos)
{
	inputState.mousePos = std::nullopt;
}

bool GraphGizmoUI::ignoreClip() const
{
	return true;
}

bool GraphGizmoUI::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::A, KeyMods::Ctrl)) {
		gizmo->addNode();
		return true;
	}

	if (key.is(KeyCode::Delete)) {
		gizmo->deleteSelection();
		return true;
	}

	if (key.is(KeyCode::C, KeyMods::Ctrl)) {
		gizmo->copySelectionToClipboard(clipboard);
		return true;
	}

	if (key.is(KeyCode::X, KeyMods::Ctrl)) {
		gizmo->cutSelectionToClipboard(clipboard);
		return true;
	}

	if (key.is(KeyCode::V, KeyMods::Ctrl)) {
		gizmo->pasteFromClipboard(clipboard);
		return true;
	}

	if (key.is(KeyCode::D, KeyMods::Ctrl)) {
		gizmo->paste(gizmo->copySelection());
		return true;
	}

	if (key.is(KeyCode::Z, KeyMods::Ctrl)) {
		graphEditor.undo();
		return true;
	}

	if (key.is(KeyCode::Z, KeyMods::CtrlShift)) {
		graphEditor.redo();
		return true;
	}

	if (key.is(KeyCode::Y, KeyMods::Ctrl)) {
		graphEditor.redo();
		return true;
	}

	return false;
}

bool GraphGizmoUI::canReceiveFocus() const
{
	return true;
}

void GraphGizmoUI::onModified()
{
	graphEditor.onModified();
}

void GraphGizmoUI::updateSelectionBox()
{
	inputState.selectionBox.reset();
	if (dragStart && inputState.mousePos) {
		const auto rect = Rect4f(*dragStart, *inputState.mousePos);
		if (rect.getWidth() >= 2 || rect.getHeight() >= 2) {
			inputState.selectionBox = rect;
		}
	}
}
