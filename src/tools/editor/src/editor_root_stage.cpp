#include "editor_root_stage.h"
#include "console/console_window.h"
#include "halley_editor.h"
#include "ui/taskbar/taskbar.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/audio/resampler.h"
#include "preferences.h"
#include "ui/editor_ui_factory.h"
#include "halley/tools/project/project.h"

using namespace Halley;

EditorRootStage::EditorRootStage(HalleyEditor& editor, std::unique_ptr<Project> project)
	: editor(editor)
	, project(std::move(project))
	, tasks(std::make_unique<EditorTaskSet>())
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
	tasks->update(time);

	updateUI(time);

	if (devConServer) {
		devConServer->update();
	}

	auto& prefs = editor.getPreferences();
	prefs.updateWindowDefinition(getVideoAPI().getWindow());
	if (prefs.isDirty()) {
		prefs.saveToFile();
	}
}

void EditorRootStage::onRender(RenderContext& context) const
{
	context.bind([&](Painter& painter)
	{
		auto view = Rect4f(painter.getViewPort());

		// Background
		Sprite bg = background;
		bg.setTexRect(view).setSize(view.getSize()).draw(painter);
		halleyLogo.clone().setPos(Vector2f(getVideoAPI().getWindow().getDefinition().getSize() / 2)).draw(painter);

		// UI
		SpritePainter spritePainter;
		spritePainter.start(100);
		ui->draw(spritePainter, 1, 0);
		spritePainter.draw(1, painter);

		// Taskbar
		taskBar->draw(painter);

		if (console) {
			console->draw(painter, Rect4f(view.getTopLeft() + Vector2f(16, 16), view.getBottomRight() - Vector2f(16, 80)));
		}
	});
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
		halleyLogo.getMaterial()
			.set("u_smoothness", 0.125f)
			.set("u_outline", 0.0f)
			.set("u_outlineColour", col);
	}
}

void EditorRootStage::createUI()
{
	uiFactory = std::make_unique<EditorUIFactory>(getAPI(), getResources(), i18n);
	ui = std::make_unique<UIRoot>(&getAudioAPI());

	uiMainPanel = std::make_shared<UIWidget>("mainPanel", Vector2f(), UISizer(UISizerType::Vertical));
	ui->addChild(uiMainPanel);

	taskBar = std::make_unique<TaskBar>(getResources());
}

void EditorRootStage::createLoadProjectUI()
{
	auto loadProjectUI = uiFactory->makeUI("ui/load_project");

	loadProjectUI->setHandle(UIEventType::ListSelectionChanged, [=] (const UIEvent& event)
	{
		event.getCurWidget().getWidgetAs<UITextInput>("input")->setText(event.getData());
	});

	loadProjectUI->setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		project = editor.loadProject(event.getCurWidget().getWidgetAs<UITextInput>("input")->getText());
		if (project) {
			event.getCurWidget().destroy();
			loadProject();
		}
	});

	auto recent = loadProjectUI->getWidgetAs<UIList>("recent");
	for (auto& r: editor.getPreferences().getRecents()) {
		recent->addTextItem(r, LocalisedString::fromUserString(r));
	}
	recent->addTextItem("", LocalisedString::fromHardcodedString("New location..."));

	uiMainPanel->add(loadProjectUI, 1, Vector4f(), UISizerAlignFlags::Centre);
}

void EditorRootStage::updateUI(Time time)
{
	if (taskBar) {
		taskBar->update(tasks->getTasks(), time);
	}

	auto kb = getInputAPI().getKeyboard();

	auto size = getVideoAPI().getWindow().getDefinition().getSize();
	uiMainPanel->setMinSize(Vector2f(size));
	ui->setRect(Rect4f(Vector2f(), Vector2f(size)));
	ui->update(time, UIInputType::Mouse, getInputAPI().getMouse(), kb);

	if (console) {
		console->update(*kb);
	}
}

void EditorRootStage::loadProject()
{
	project->setDevConServer(devConServer.get());
	tasks->addTask(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(*project, false)));

	console = std::make_unique<ConsoleWindow>(getResources());
}
