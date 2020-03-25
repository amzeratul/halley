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
}

SceneEditorCanvas::~SceneEditorCanvas()
{
	unloadDLL();
}

void SceneEditorCanvas::update(Time t, bool moved)
{
	updateInterface(t);
	updateCanvas(Vector2i(getSize()) - Vector2i(2, 2));
}

void SceneEditorCanvas::draw(UIPainter& painter) const
{
	const auto pos = getPosition();
	const auto size = getSize();
	painter.draw(border.clone().setPos(pos).setSize(Vector2f(size.x, 1)), true);
	painter.draw(border.clone().setPos(pos + Vector2f(0, size.y - 1)).setSize(Vector2f(size.x, 1)), true);
	painter.draw(border.clone().setPos(pos).setSize(Vector2f(1, size.y)), true);
	painter.draw(border.clone().setPos(pos + Vector2f(size.x - 1, 0)).setSize(Vector2f(1, size.y)), true);

	auto canvas = Sprite()
		.setPos(getPosition() + Vector2f(1, 1)).setSize(getSize() - Vector2f(2, 2));

	if (interface && renderTarget) {
		const auto& tex = renderTarget->getTexture(0);
		canvas
			.setImage(tex, resources.get<MaterialDefinition>("Halley/SpriteOpaque"))
			.setSize(Vector2f(curRenderSize))
			.setTexRect(Rect4f(Vector2f(), Vector2f(curRenderSize) / Vector2f(tex->getSize())));
	} else {
		canvas.setImage(resources, "whitebox.png").setColour(Colour4f(0.2f, 0.2f, 0.2f));
	}
	
	painter.draw(canvas, true);
}

void SceneEditorCanvas::render(RenderContext& rc) const
{
	renderInterface(rc);
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
	
	if (interface) {
		guardedRun([&]() {
			auto context = rc.with(*renderTarget);
			interface->render(context);
		});
	}
}

void SceneEditorCanvas::updateCanvas(Vector2i size)
{
	if (size != curRenderSize && size.x > 0 && size.y > 0) {
		curRenderSize = size;

		const auto textureSize = Vector2i(nextPowerOf2(size.x), nextPowerOf2(size.y));
		if (textureSize != curTextureSize) {
			curTextureSize = textureSize;
			
			std::shared_ptr<Texture> colourTarget = api.video->createTexture(textureSize);
			auto colourDesc = TextureDescriptor(textureSize, TextureFormat::RGBA);
			colourDesc.isRenderTarget = true;
			colourTarget->load(std::move(colourDesc));

			std::shared_ptr<Texture> depthTarget = api.video->createTexture(textureSize);
			auto depthDesc = TextureDescriptor(textureSize, TextureFormat::DEPTH);
			depthDesc.isDepthStencil = true;
			depthTarget->load(std::move(depthDesc));

			renderTarget = api.video->createTextureRenderTarget();
			renderTarget->setTarget(0, colourTarget);
			renderTarget->setDepthTexture(depthTarget);
		}

		renderTarget->setViewPort(Rect4i(Vector2i(), size));
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
	Expects(gameDLL);
	interface.reset();
	gameDLL->unload();

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
