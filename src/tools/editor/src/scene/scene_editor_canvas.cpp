#include "scene_editor_canvas.h"
#include "halley/core/game/scene_editor_interface.h"
#include "scene_editor_window.h"
#include "src/project/core_api_wrapper.h"

using namespace Halley;

SceneEditorCanvas::SceneEditorCanvas(String id, Resources& resources, const HalleyAPI& api)
	: UIWidget(std::move(id))
	, api(api)
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

SceneEditorCanvas::~SceneEditorCanvas()
{
	unloadDLL();
}

void SceneEditorCanvas::update(Time t, bool moved)
{
	updateInterface(t);
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

	if (interface && surface->isReady()) {
		canvas = surface->getSurfaceSprite();
	} else {
		canvas.setImage(resources, "whitebox.png").setColour(Colour4f(0.2f, 0.2f, 0.2f));
	}

	canvas.setPos(getPosition() + Vector2f(1, 1)).setSize(getSize() - Vector2f(2, 2));
	
	painter.draw(canvas, true);
}

void SceneEditorCanvas::render(RenderContext& rc) const
{
	renderInterface(rc);
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
		if (button == 1) {
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

	if (dragging && interface) {
		interface->dragCamera(lastMousePos - mousePos);
	}
	lastMousePos = mousePos;
}

void SceneEditorCanvas::onMouseWheel(const UIEvent& event)
{
	if (interface) {
		interface->changeZoom(event.getIntData(), lastMousePos - getPosition() - getSize() / 2);
	}
}

void SceneEditorCanvas::loadGame(std::shared_ptr<DynamicLibrary> dll, Resources& gameResources)
{
	this->gameResources = &gameResources;
	gameDLL = std::move(dll);
	if (gameDLL) {
		loadDLL();
	}
}

bool SceneEditorCanvas::needsReload() const
{
	if (gameDLL) {
		return gameDLL->hasChanged();
	}
	return false;
}

void SceneEditorCanvas::reload()
{
	reloadDLL();
}

bool SceneEditorCanvas::isLoaded() const
{
	return static_cast<bool>(interface);
}

ISceneEditor& SceneEditorCanvas::getInterface() const
{
	Expects(interface);
	return *interface;
}

void SceneEditorCanvas::setSceneEditorWindow(SceneEditorWindow& window)
{
	editorWindow = &window;
}

void SceneEditorCanvas::updateInterface(Time t)
{
	if (errorState) {
		unloadDLL();
	}

	if (interface) {
		updateInputState();
		guardedRun([&] () {
			interface->update(t, inputState, outputState);
		});
		notifyOutputState();
		clearInputState();
	}
}

void SceneEditorCanvas::renderInterface(RenderContext& rc) const
{
	if (errorState) {
		return;
	}
	
	if (interface && surface->isReady()) {
		guardedRun([&]() {
			auto context = rc.with(surface->getRenderTarget());
			interface->render(context);
		});
	}
}

void SceneEditorCanvas::loadDLL()
{
	Expects(gameDLL);

	gameDLL->load(true);
	auto getHalleyEntry = reinterpret_cast<IHalleyEntryPoint * (HALLEY_STDCALL*)()>(gameDLL->getFunction("getHalleyEntry"));
	auto game = getHalleyEntry()->createGame();
	guardedRun([&]() {
		interface = game->createSceneEditorInterface();
	});
	game.reset();

	if (interface) {
		gameCoreAPI = std::make_unique<CoreAPIWrapper>(*api.core);
		gameAPI = api.clone();
		gameAPI->replaceCoreAPI(gameCoreAPI.get());
		
		SceneEditorContext context;
		context.resources = gameResources;
		context.editorResources = &resources;
		context.api = gameAPI.get();

		guardedRun([&]() {
			interface->init(context);
		});
		if (errorState) {
			unloadDLL();
		}
	}
}

void SceneEditorCanvas::unloadDLL()
{
	interface.reset();

	if (gameDLL) {
		gameDLL->unload();
	}

	gameAPI.reset();
	gameCoreAPI.reset();

	errorState = false;
}

void SceneEditorCanvas::reloadDLL()
{
	// TODO
	Logger::logWarning("SceneEditorCanvas::reloadDLL() not implemented yet");
}

void SceneEditorCanvas::guardedRun(const std::function<void()>& f) const
{
	try {
		f();
	} catch (const std::exception& e) {
		Logger::logException(e);
		errorState = true;
	} catch (...) {
		Logger::logError("Unknown error in SceneEditorCanvas, probably from game dll");
		errorState = true;
	}
}

void SceneEditorCanvas::updateInputState()
{
	inputState.ctrlHeld = keyboard->isButtonDown(Keys::LCtrl) || keyboard->isButtonDown(Keys::RCtrl);
	inputState.shiftHeld = keyboard->isButtonDown(Keys::LShift) || keyboard->isButtonDown(Keys::RShift);

	inputState.viewRect = getRect();
}

void SceneEditorCanvas::notifyOutputState()
{
	for (auto& m: outputState.fieldsChanged) {
		editorWindow->onFieldChangedByGizmo(m.first, m.second);
	}
	outputState.fieldsChanged.clear();
}

void SceneEditorCanvas::clearInputState()
{
	inputState.leftClickPressed = false;
	inputState.middleClickPressed = false;
	inputState.rightClickPressed = false;
}
