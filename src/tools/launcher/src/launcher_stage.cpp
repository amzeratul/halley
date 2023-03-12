#include "launcher_stage.h"

#include "choose_project.h"
#include "launcher_save_data.h"
using namespace Halley;

LauncherStage::LauncherStage()
	: mainThreadExecutor(Executors::getMainUpdateThread())
{
}

void LauncherStage::init()
{
	saveData = std::make_shared<LauncherSaveData>(getSystemAPI().getStorageContainer(SaveDataType::SaveLocal));
	
	makeUI();
	makeSprites();
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
		background.clone().setTexRect(view).setSize(view.getSize()).draw(painter);

		// Logo
		halleyLogo.clone().setPosition(view.getTopCenter() + Vector2f(0, 40)).draw(painter);
		
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
	background = uiFactory->getColourScheme()->getBackground();

	const auto col = uiFactory->getColourScheme()->getColour("logo");
	halleyLogo = Sprite()
		.setImage(getResources(), "halley/halley_logo_dist.png", "Halley/DistanceFieldSprite")
		.setPivot(Vector2f(0.5f, 0.5f))
		.setScale(Vector2f(0.8f, 0.8f))
		.setColour(col);
	halleyLogo.getMutableMaterial()
		.set("u_smoothness", 16.0f)
		.set("u_outline", 0.0f)
		.set("u_outlineColour", col);
}

void LauncherStage::makeUI()
{
	uiFactory = getGame().createUIFactory(getAPI(), getResources(), i18n);

	ui = std::make_unique<UIRoot>(getAPI());
	ui->makeToolTip(uiFactory->getStyle("tooltip"));

	topLevelUI = uiFactory->makeUI("launcher/background");
	ui->addChild(topLevelUI);

	setCurrentUI(std::make_shared<ChooseProject>(*uiFactory));

	const auto bgCol = uiFactory->getColourScheme()->getColour("background0");
	getVideoAPI().getWindow().setTitleColour(bgCol, bgCol);
}

void LauncherStage::updateUI(Time time)
{
	const auto kb = getInputAPI().getKeyboard();
	const auto size = getVideoAPI().getWindow().getDefinition().getSize();
	const auto uiRect = Rect4f(Vector2f(), Vector2f(size)).grow(0, -60, 0, 0);

	topLevelUI->setMinSize(uiRect.getSize());
	topLevelUI->setPosition(uiRect.getTopLeft());
	ui->setRect(uiRect);
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
