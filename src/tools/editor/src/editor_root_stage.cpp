#include "editor_root_stage.h"
#include "halley_editor.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/audio/resampler.h"
#include "preferences.h"
#include "halley/tools/project/project.h"
#include "ui/editor_ui_factory.h"
#include "ui/console_window.h"
#include "ui/load_project_window.h"
#include "assets/assets_browser.h"
#include "scene/scene_editor_window.h"
#include "ui/project_window.h"

using namespace Halley;

EditorRootStage::EditorRootStage(HalleyEditor& editor, std::unique_ptr<Project> project)
	: editor(editor)
	, mainThreadExecutor(Executors::getMainUpdateThread())
	, project(std::move(project))
{
}

EditorRootStage::~EditorRootStage()
{
	topLevelUI.reset();
	projectWindow.reset();
	ui.reset();
	uiFactory.reset();
	devConServer.reset();

	project.reset();
}

void EditorRootStage::init()
{
	createUI();
	initSprites();

	devConServer = std::make_unique<DevConServer>(getNetworkAPI().createService(NetworkProtocol::TCP, DevCon::devConPort), DevCon::devConPort);

	if (project) {
		loadProject();
	} else {
		createLoadProjectUI();
	}
}

void EditorRootStage::onVariableUpdate(Time time)
{
	mainThreadExecutor.runPending();

	if (!topLevelUI || !topLevelUI->isAlive()) {
		unloadProject();
		createLoadProjectUI();
	}

	updateUI(time);

	if (devConServer) {
		devConServer->update(time);
	}

	auto& prefs = editor.getPreferences();
	prefs.updateWindowDefinition(getVideoAPI().getWindow());
	if (prefs.isDirty()) {
		prefs.saveToFile(getSystemAPI());
	}

	if (project) {
		project->update(time);
	}
}

void EditorRootStage::onRender(RenderContext& context) const
{
	ui->render(context);
	
	context.bind([&](Painter& painter)
	{
		painter.clear(Colour4f()); // Needed for depth/stencil
		auto view = Rect4f(painter.getViewPort());

		// Background
		background
			.clone()
			.setTexRect(view)
			.setSize(view.getSize())
			.draw(painter);

		if (!topLevelUI || !std::dynamic_pointer_cast<LoadProjectWindow>(topLevelUI)) {
			halleyLogo.clone().setPos(Vector2f(getVideoAPI().getWindow().getDefinition().getSize() / 2)).draw(painter);
		}

		Sprite::draw(backgroundParticles.getSprites(), painter);

		// UI
		SpritePainter spritePainter;
		spritePainter.start();
		ui->draw(spritePainter, 1, 0);
		spritePainter.draw(1, painter);

		// Cursor
		if (softCursor) {
			cursor
				.clone()
				.setPosition(getInputAPI().getMouse(0)->getPosition())
				.draw(painter);
		}
	});
}

void EditorRootStage::setSoftCursor(bool enabled)
{
	softCursor = enabled;
	if (enabled) {
		cursor = Sprite().setImage(getResources(), "ui/cursor.png").setAbsolutePivot(Vector2f(2, 2));
	}
	getSystemAPI().showCursor(!enabled);
}

bool EditorRootStage::isSoftCursor() const
{
	return softCursor;
}

bool EditorRootStage::onQuitRequested()
{
	return !projectWindow || projectWindow->onQuitRequested();
}

void EditorRootStage::initSprites()
{
	// Colour scheme
	const auto& colourScheme = uiFactory->getColourScheme();
	background = colourScheme->getBackground();
	backgroundParticles = colourScheme->getBackgroundParticles();

	{
		// Halley logo
		auto col = colourScheme->getColour("backgroundLogo");
		halleyLogo = Sprite()
			.setImage(getResources(), "halley/halley_icon_dist.png", "Halley/DistanceFieldSprite")
			.setPivot(Vector2f(0.5f, 0.5f))
			.setColour(col)
			.setScale(Vector2f(8, 8))
			.setPos(Vector2f(640, 360));
		halleyLogo.getMutableMaterial()
			.set("u_smoothness", 0.125f)
			.set("u_outline", 0.0f)
			.set("u_outlineColour", col);
	}
}

void EditorRootStage::createUI()
{
	uiFactory = std::make_unique<EditorUIFactory>(getAPI(), getResources(), i18n, editor.getPreferences().getColourScheme());
	ui = std::make_unique<UIRoot>(getAPI());
	ui->makeToolTip(uiFactory->getStyle("tooltip"));

	ui->setUnhandledKeyPressListener([=] (KeyboardKeyPress key) -> bool
	{
		if (key.is(KeyCode::F2)) {
			setSoftCursor(!isSoftCursor());
			return true;
		}
		
		return false;
	});
}

void EditorRootStage::createLoadProjectUI()
{
	setTopLevelUI(std::make_shared<LoadProjectWindow>(*uiFactory, editor, [this] (String str)
	{
		Concurrent::execute(Executors::getMainUpdateThread(), [=] () {
			project = editor.loadProject(str);
			if (project) {
				loadProject();
			} else {
				createLoadProjectUI();
			}
		});
	}));
}

void EditorRootStage::updateUI(Time time)
{
	const auto kb = getInputAPI().getKeyboard();
	const auto size = getVideoAPI().getWindow().getDefinition().getSize();

	//uiMainPanel->setMinSize(Vector2f(size));
	ui->setRect(Rect4f(Vector2f(), Vector2f(size)));
	ui->update(time, UIInputType::Mouse, getInputAPI().getMouse(), kb);

	backgroundParticles.setPosition(Vector2f(size) * Vector2f(0.6f, 0.4f));
	backgroundParticles.setSpawnArea(Vector2f(size) * 1.2f);
	backgroundParticles.update(time);
}

void EditorRootStage::loadProject()
{
	project->setDevConServer(devConServer.get());

	projectWindow = std::make_shared<ProjectWindow>(*uiFactory, editor, *project, getResources(), getAPI());
	setTopLevelUI(projectWindow);
}

void EditorRootStage::unloadProject()
{
	initSprites();
	setTopLevelUI({});
	projectWindow.reset();
	project.reset();
}

void EditorRootStage::setTopLevelUI(std::shared_ptr<UIWidget> uiWindow)
{
	if (topLevelUI) {
		topLevelUI->destroy();
	}
	topLevelUI = std::move(uiWindow);
	if (topLevelUI) {
		ui->addChild(topLevelUI);
		ui->registerKeyPressListener(topLevelUI, 0);
	}
}
