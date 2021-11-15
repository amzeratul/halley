#include "launcher_stage.h"

#include "choose_project.h"
#include "launcher_save_data.h"
using namespace Halley;

LauncherStage::LauncherStage()
	: mainThreadExecutor(Executors::getMainThread())
{
}

void LauncherStage::init()
{
	saveData = std::make_shared<LauncherSaveData>(getSystemAPI().getStorageContainer(SaveDataType::SaveLocal));
	
	makeSprites();
	makeUI();
}

void LauncherStage::onVariableUpdate(Time time)
{
	mainThreadExecutor.runPending();
	updateUI(time);
}

void LauncherStage::onRender(RenderContext& context) const
{
	ui->render(context);

	context.bind([&](Painter& painter)
	{
		painter.clear(Colour4f()); // Needed for depth/stencil
		auto view = Rect4f(painter.getViewPort());

		// Background
		Sprite bg = background;
		bg.setTexRect(view).setSize(view.getSize()).draw(painter);
		
		// UI
		SpritePainter spritePainter;
		spritePainter.start();
		ui->draw(spritePainter, 1, 0);
		spritePainter.draw(1, painter);
	});
}

void LauncherStage::makeSprites()
{
	auto mat = std::make_shared<Material>(getResource<MaterialDefinition>("Launcher/Background"));
	mat
		->set("u_col0", Colour4f(0.08f))
		.set("u_col1", Colour4f(0.07f))
		.set("u_distance", 6.0f);
	background = Sprite().setMaterial(mat).setPos(Vector2f(0, 0));
}

void LauncherStage::makeUI()
{
	auto styleSheet = std::make_shared<UIStyleSheet>(getResources());
	for (auto& style: getResources().enumerate<ConfigFile>()) {
		if (style.startsWith("ui_style/")) {
			styleSheet->load(*getResources().get<ConfigFile>(style));
		}
	}

	uiFactory = std::make_unique<UIFactory>(getAPI(), getResources(), i18n, styleSheet);
	ui = std::make_unique<UIRoot>(getAPI());
	ui->makeToolTip(uiFactory->getStyle("tooltip"));

	topLevelUI = uiFactory->makeUI("background");
	ui->addChild(topLevelUI);

	setCurrentUI(std::make_shared<ChooseProject>(*uiFactory));
}

void LauncherStage::updateUI(Time time)
{
	const auto kb = getInputAPI().getKeyboard();
	const auto size = getVideoAPI().getWindow().getDefinition().getSize();
	topLevelUI->setMinSize(Vector2f(size));
	ui->setRect(Rect4f(Vector2f(), Vector2f(size)));
	ui->update(time, UIInputType::Mouse, getInputAPI().getMouse(), kb);
}

void LauncherStage::setCurrentUI(std::shared_ptr<UIWidget> ui)
{
	auto container = topLevelUI->getWidget("container");
	container->clear();
	if (curUI) {
		curUI->destroy();
	}
	container->add(ui, 1);
	curUI = ui;
}
