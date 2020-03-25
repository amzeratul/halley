#include "scene_editor_canvas.h"
#include "halley/core/game/scene_editor_interface.h"

using namespace Halley;

SceneEditorCanvas::SceneEditorCanvas(String id, Resources& resources, const HalleyAPI& api)
	: UIWidget(std::move(id))
	, api(api)
	, resources(resources)
{
	border.setImage(resources, "whitebox.png").setColour(Colour4f());
	canvas.setImage(resources, "whitebox.png").setColour(Colour4f(0.2f, 0.2f, 0.2f));
}

SceneEditorCanvas::~SceneEditorCanvas() = default;

void SceneEditorCanvas::update(Time t, bool moved)
{
	canvas.setPos(getPosition() + Vector2f(1, 1)).setSize(getSize() - Vector2f(2, 2));
	updateInterface(t);
}

void SceneEditorCanvas::draw(UIPainter& painter) const
{
	const auto pos = getPosition();
	const auto size = getSize();
	painter.draw(border.clone().setPos(pos).setSize(Vector2f(size.x, 1)), true);
	painter.draw(border.clone().setPos(pos + Vector2f(0, size.y - 1)).setSize(Vector2f(size.x, 1)), true);
	painter.draw(border.clone().setPos(pos).setSize(Vector2f(1, size.y)), true);
	painter.draw(border.clone().setPos(pos + Vector2f(size.x - 1, 0)).setSize(Vector2f(1, size.y)), true);

	renderInterface();
	
	painter.draw(canvas);
}

void SceneEditorCanvas::setGameDLL(std::shared_ptr<DynamicLibrary> dll, Resources& gameResources)
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

void SceneEditorCanvas::updateInterface(Time t)
{
	if (interface) {
		interface->update(t);
	}
}

void SceneEditorCanvas::renderInterface() const
{
	if (interface) {
		// TODO
	}
}

void SceneEditorCanvas::loadDLL()
{
	Expects(gameDLL);

	gameDLL->load(true);
	auto getHalleyEntry = reinterpret_cast<IHalleyEntryPoint * (HALLEY_STDCALL*)()>(gameDLL->getFunction("getHalleyEntry"));
	auto game = getHalleyEntry()->createGame();
	interface = game->createSceneEditorInterface();

	if (interface) {
		SceneEditorContext context;
		context.resources = gameResources;
		context.api = &api;
		interface->init(context);
	}
}

void SceneEditorCanvas::unloadDLL()
{
	Expects(gameDLL);
	interface.reset();
	gameDLL->unload();
}

void SceneEditorCanvas::reloadDLL()
{
	// TODO
	Logger::logWarning("SceneEditorCanvas::reloadDLL() not implemented yet");
}
