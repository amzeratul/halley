#include "scene_editor_canvas.h"
#include "halley/core/game/scene_editor_interface.h"
#include "src/project/core_api_wrapper.h"

using namespace Halley;

SceneEditorCanvas::SceneEditorCanvas(String id, Resources& resources, const HalleyAPI& api)
	: UIWidget(std::move(id))
	, api(api)
	, resources(resources)
{
	border.setImage(resources, "whitebox.png").setColour(Colour4f());
	surface = std::make_shared<RenderSurface>(*api.video, resources);
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
	if (button == 0) {
		dragging = true;
		lastMousePos = mousePos;
	}
}

void SceneEditorCanvas::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		if (dragging) {
			onMouseOver(mousePos);
			dragging = false;
		}
	}
}

void SceneEditorCanvas::onMouseOver(Vector2f mousePos)
{
	if (dragging) {
		interface->dragCamera(lastMousePos - mousePos);
		lastMousePos = mousePos;
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

SceneEditorInterface& SceneEditorCanvas::getInterface() const
{
	Expects(interface);
	return *interface;
}

void SceneEditorCanvas::updateInterface(Time t)
{
	if (errorState) {
		unloadDLL();
	}
	
	if (interface) {
		guardedRun([&] () {
			interface->update(t);
		});
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
