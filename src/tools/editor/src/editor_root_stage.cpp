#include "editor_root_stage.h"
#include "halley_editor.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/audio/resampler.h"
#include "preferences.h"
#include "halley/tools/project/project.h"
#include "ui/editor_ui_factory.h"
#include "ui/console_window.h"
#include "ui/load_project_window.h"
#include "ui/taskbar.h"
#include "ui/toolbar.h"
#include "assets/assets_editor_window.h"

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
	if (tasks) {
		tasks->update(time);
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
		spritePainter.start();
		ui->draw(spritePainter, 1, 0);
		spritePainter.draw(1, painter);
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

void EditorRootStage::clearUI()
{
	if (uiTop) {
		uiTop->clear();
	}
	if (uiMid) {
		uiMid->clear();
	}
	if (uiBottom) {
		uiBottom->clear();
	}
	pagedPane = {};
}

void EditorRootStage::createUI()
{
	uiFactory = std::make_unique<EditorUIFactory>(getAPI(), getResources(), i18n);
	ui = std::make_unique<UIRoot>(&getAudioAPI());

	uiMainPanel = std::make_shared<UIWidget>("uiMainPanel", Vector2f(), UISizer(UISizerType::Vertical));
	ui->addChild(uiMainPanel);

	uiTop = std::make_shared<UIWidget>("uiTop", Vector2f(), UISizer(UISizerType::Horizontal));
	uiMid = std::make_shared<UIWidget>("uiMid", Vector2f(), UISizer(UISizerType::Horizontal));
	uiBottom = std::make_shared<UIWidget>("uiBottom", Vector2f(), UISizer(UISizerType::Horizontal));
	uiMainPanel->add(uiTop);
	uiMainPanel->add(uiMid, 1);
	uiMainPanel->add(uiBottom);
}

void EditorRootStage::createLoadProjectUI()
{
	clearUI();
	unloadProject();

	uiMid->add(std::make_shared<LoadProjectWindow>(*uiFactory, editor, [this] (String str)
	{
		Concurrent::execute(Executors::getMainThread(), [=] () {
			project = editor.loadProject(str);
			if (project) {
				loadProject();
			} else {
				createLoadProjectUI();
			}
		});
	}), 1, Vector4f(), UISizerAlignFlags::Centre);
}

void EditorRootStage::createProjectUI()
{
	clearUI();

	pagedPane = std::make_shared<UIPagedPane>("pages", 5);
	const auto toolbar = std::make_shared<Toolbar>(*uiFactory, project->getProperties());
	const auto taskbar = std::make_shared<TaskBar>(*uiFactory, *tasks);

	uiTop->add(toolbar, 1, Vector4f(0, 16, 0, 8));
	uiMid->add(pagedPane, 1);
	uiBottom->add(taskbar, 1);

	pagedPane->getPage(0)->add(std::make_shared<AssetsEditorWindow>(*uiFactory, *project, getAPI()), 1, Vector4f(8, 8, 8, 8));
	pagedPane->getPage(4)->add(std::make_shared<ConsoleWindow>(*uiFactory), 1, Vector4f(8, 8, 8, 8));

	toolbar->setHandle(UIEventType::ListSelectionChanged, [=] (const UIEvent& event)
	{
		String toolName;
		switch (event.getIntData()) {
		case 0:
			toolName = "Assets";
			break;
		case 1:
			toolName = "ECS";
			break;
		case 2:
			toolName = "Remotes";
			break;
		case 3:
			toolName = "Properties";
			break;
		case 4:
			toolName = "Settings";
			break;
		}
		toolbar->getWidgetAs<UILabel>("toolName")->setText(LocalisedString::fromHardcodedString(toolName));
		pagedPane->setPage(event.getIntData());
	});
	toolbar->setHandle(UIEventType::ButtonClicked, "exitProject", [=] (const UIEvent& event)
	{
		createLoadProjectUI();
	});
}

void EditorRootStage::updateUI(Time time)
{
	const auto kb = getInputAPI().getKeyboard();
	const auto size = getVideoAPI().getWindow().getDefinition().getSize();

	uiMainPanel->setMinSize(Vector2f(size));
	ui->setRect(Rect4f(Vector2f(), Vector2f(size)));
	ui->update(time, UIInputType::Mouse, getInputAPI().getMouse(), kb);
}

void EditorRootStage::loadProject()
{
	project->setDevConServer(devConServer.get());
	tasks = std::make_unique<EditorTaskSet>();
	tasks->addTask(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(*project, false)));

	createProjectUI();
}

void EditorRootStage::unloadProject()
{
	tasks.reset();
	project.reset();
}
