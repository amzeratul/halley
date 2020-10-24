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
	, mainThreadExecutor(Executors::getMainThread())
	, project(std::move(project))
{
}

EditorRootStage::~EditorRootStage()
{
}

void EditorRootStage::init()
{
	initSprites();

	createUI();

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
		devConServer->update();
	}

	auto& prefs = editor.getPreferences();
	prefs.updateWindowDefinition(getVideoAPI().getWindow());
	if (prefs.isDirty()) {
		prefs.saveToFile();
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
		Sprite bg = background;
		bg.setTexRect(view).setSize(view.getSize()).draw(painter);
		halleyLogo.clone().setPos(Vector2f(getVideoAPI().getWindow().getDefinition().getSize() / 2)).draw(painter);

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

void EditorRootStage::initSprites()
{
	{
		// Background
		{
			auto mat = std::make_shared<Material>(getResource<MaterialDefinition>("Halley/Scanlines"));
			mat
				->set("u_col0", Colour4f(0.08f))
				.set("u_col1", Colour4f(0.07f))
				.set("u_distance", 6.0f);
			background = Sprite().setMaterial(mat).setPos(Vector2f(0, 0));
		}
	}

	{
		// Halley logo
		auto col = Colour4f(0.065f);
		//auto col = Colour4f(0.9882f, 0.15686f, 0.27843f, 1);
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
	uiFactory = std::make_unique<EditorUIFactory>(getAPI(), getResources(), i18n);
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
		Concurrent::execute(Executors::getMainThread(), [=] () {
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
}

void EditorRootStage::loadProject()
{
	project->setDevConServer(devConServer.get());

	setTopLevelUI(std::make_shared<ProjectWindow>(*uiFactory, editor, *project, getResources(), getAPI()));
}

void EditorRootStage::unloadProject()
{
	setTopLevelUI({});
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
	}
}
