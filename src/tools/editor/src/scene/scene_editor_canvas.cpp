#include "scene_editor_canvas.h"


#include "scene_editor_game_bridge.h"
#include "halley/core/entry/entry_point.h"
#include "halley/core/game/game.h"
#include "halley/core/graphics/render_context.h"
#include "halley/core/graphics/render_target/render_surface.h"
#include "halley/core/graphics/render_target/render_target_texture.h"
#include "halley/core/input/input_keyboard.h"
#include "halley/core/input/input_keys.h"
#include "scene_editor_gizmo_collection.h"
#include "halley/core/game/scene_editor_interface.h"
#include "scene_editor_window.h"
#include "src/project/core_api_wrapper.h"

using namespace Halley;

SceneEditorCanvas::SceneEditorCanvas(String id, UIFactory& factory, Resources& resources, const HalleyAPI& api, std::optional<UISizer> sizer)
	: UIWidget(std::move(id), Vector2f(32, 32), std::move(sizer))
	, resources(resources)
{
	border.setImage(resources, "whitebox.png").setColour(Colour4f());
	surface = std::make_shared<RenderSurface>(*api.video, resources);

	keyboard = api.input->getKeyboard();
	mouse = api.input->getMouse();

	setHandle(UIEventType::MouseWheel, [this](const UIEvent& event)
	{
		onMouseWheel(event);
	});
}

void SceneEditorCanvas::update(Time t, bool moved)
{
	updateInputState();
	if (gameBridge) {
		outputState.clear();
		gameBridge->update(t, inputState, outputState);
	}
	notifyOutputState();
	clearInputState();

	surface->setSize(Vector2i(getSize()) - Vector2i(2, 2));
}

void SceneEditorCanvas::draw(UIPainter& painter) const
{
	const auto pos = getPosition();
	const auto size = getSize();
	painter.draw(border.clone().setPos(pos).setSize(Vector2f(size.x, 1)), true);
	painter.draw(border.clone().setPos(pos + Vector2f(0, size.y - 1)).setSize(Vector2f(size.x, 1)), true);
	painter.draw(border.clone().setPos(pos).setSize(Vector2f(1, size.y)), true);
	painter.draw(border.clone().setPos(pos + Vector2f(size.x - 1, 0)).setSize(Vector2f(1, size.y)), true);

	Sprite canvas;

	if (surface->isReady()) {
		canvas = surface->getSurfaceSprite();
	} else {
		canvas.setImage(resources, "whitebox.png").setColour(Colour4f(0.2f, 0.2f, 0.2f));
	}

	canvas.setPos(getPosition() + Vector2f(1, 1)).setSize(getSize() - Vector2f(2, 2));

	painter.draw(canvas, true);
}

void SceneEditorCanvas::render(RenderContext& rc) const
{
	if (gameBridge && surface->isReady()) {
		auto context = rc.with(surface->getRenderTarget());
		gameBridge->render(context);
	}
}

bool SceneEditorCanvas::canInteractWithMouse() const
{
	return true;
}

bool SceneEditorCanvas::isFocusLocked() const
{
	return dragging;
}

void SceneEditorCanvas::pressMouse(Vector2f mousePos, int button)
{
	switch (button) {
	case 0:
		inputState.leftClickPressed = inputState.leftClickHeld = true;
		break;
	case 1:
		inputState.middleClickPressed = inputState.middleClickHeld = true;
		break;
	case 2:
		inputState.rightClickPressed = inputState.rightClickHeld = true;
		break;
	}

	if (!dragging) {
		if (button == 1 || (tool == "drag" && button == 0) || (inputState.spaceHeld && button == 0)) {
			dragButton = button;
			dragging = true;
			lastMousePos = mousePos;
		}
	}
}

void SceneEditorCanvas::releaseMouse(Vector2f mousePos, int button)
{
	switch (button) {
	case 0:
		inputState.leftClickReleased = true;
		inputState.leftClickHeld = false;
		break;
	case 1:
		inputState.middleClickHeld = false;
		break;
	case 2:
		inputState.rightClickHeld = false;
		break;
	}

	if (button == dragButton && dragging) {
		onMouseOver(mousePos);
		dragging = false;
	}
}

void SceneEditorCanvas::onMouseOver(Vector2f mousePos)
{
	inputState.rawMousePos = mousePos;

	if (dragging && gameBridge) {
		gameBridge->dragCamera(lastMousePos - mousePos);
	}
	lastMousePos = mousePos;
}

void SceneEditorCanvas::onMouseWheel(const UIEvent& event)
{
	gameBridge->changeZoom(event.getIntData(), lastMousePos - getPosition() - getSize() / 2);
}

void SceneEditorCanvas::setGameBridge(SceneEditorGameBridge& bridge)
{
	gameBridge = &bridge;
}

void SceneEditorCanvas::setSceneEditorWindow(SceneEditorWindow& window)
{
	editorWindow = &window;
}

std::shared_ptr<UIWidget> SceneEditorCanvas::setTool(const String& tool, const String& componentName, const String& fieldName, const ConfigNode& options)
{
	this->tool = tool;
	return gameBridge->getGizmos().setTool(tool, componentName, fieldName, options);
}

void SceneEditorCanvas::updateInputState()
{
	inputState.ctrlHeld = keyboard->isButtonDown(KeyCode::LCtrl) || keyboard->isButtonDown(KeyCode::RCtrl);
	inputState.shiftHeld = keyboard->isButtonDown(KeyCode::LShift) || keyboard->isButtonDown(KeyCode::RShift);
	inputState.spaceHeld = keyboard->isButtonDown(KeyCode::Space);

	inputState.viewRect = getRect();
}

void SceneEditorCanvas::notifyOutputState()
{
	for (auto& m: outputState.fieldsChanged) {
		editorWindow->onFieldChangedByGizmo(m.first, m.second);
	}
	outputState.fieldsChanged.clear();

	if (outputState.newSelection) {
		editorWindow->selectEntity(outputState.newSelection.value());
	}
}

void SceneEditorCanvas::clearInputState()
{
	inputState.leftClickPressed = false;
	inputState.leftClickReleased = false;
	inputState.middleClickPressed = false;
	inputState.rightClickPressed = false;
}
