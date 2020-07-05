#include "launcher_stage.h"
using namespace Halley;

void LauncherStage::init()
{
	makeUI();
}

void LauncherStage::onVariableUpdate(Time time)
{
	updateUI(time);
}

void LauncherStage::onRender(RenderContext& context) const
{
	ui->render(context);

	context.bind([&](Painter& painter)
	{
		painter.clear(Colour4f()); // Needed for depth/stencil
		auto view = Rect4f(painter.getViewPort());

		// UI
		SpritePainter spritePainter;
		spritePainter.start();
		ui->draw(spritePainter, 1, 0);
		spritePainter.draw(1, painter);
	});
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

	topLevelUI = uiFactory->makeUI("ui/load_project");
	topLevelUI->setAnchor(UIAnchor());
	ui->addChild(topLevelUI);
}

void LauncherStage::updateUI(Time time)
{
	const auto kb = getInputAPI().getKeyboard();
	const auto size = getVideoAPI().getWindow().getDefinition().getSize();
	ui->setRect(Rect4f(Vector2f(), Vector2f(size)));
	ui->update(time, UIInputType::Mouse, getInputAPI().getMouse(), kb);
}
