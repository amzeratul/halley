#include "ui_editor_display.h"

#include "ui_editor.h"

using namespace Halley;

UIEditorDisplay::UIEditorDisplay(String id, Vector2f minSize, UISizer sizer)
	: UIWidget(std::move(id), minSize, std::move(sizer))
{
}

void UIEditorDisplay::setUIEditor(UIEditor& uiEditor)
{
	factory = &uiEditor.getGameFactory();
	factory->setConstructionCallback([=] (const std::shared_ptr<UIWidget>& widget, const String& uuid)
	{
		if (!uuid.isEmpty()) {
			widgets[UUID(uuid)] = widget;
		}
		maxAdjustment = std::max(maxAdjustment, widget->getChildLayerAdjustment());
	});

	boundsSprite.setImage(factory->getResources(), "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	sizerSprite.setImage(factory->getResources(), "whitebox_outline.png").setColour(Colour4f(0.7f, 0.7f, 0.7f));
}

void UIEditorDisplay::onMakeUI()
{
	updateCurWidget();
}

void UIEditorDisplay::drawAfterChildren(UIPainter& painter) const
{
	if (curWidget) {
		auto p = painter.withAdjustedLayer(maxAdjustment + 1);

		for (const auto& s: sizerSprites) {
			p.draw(s);
		}

		p.draw(boundsSprite);
	}
}

void UIEditorDisplay::update(Time time, bool moved)
{
	sizerSprites.clear();

	if (curWidget) {
		boundsSprite
			.setPosition(curWidget->getPosition())
			.scaleTo(curWidget->getSize());
	}
}

void UIEditorDisplay::setSelectedWidget(const String& id)
{
	curSelection = id;
	updateCurWidget();
}

void UIEditorDisplay::loadDisplay(const UIDefinition& uiDefinition)
{
	clear();
	widgets.clear();
	maxAdjustment = 0;

	factory->loadUI(*this, uiDefinition);
}

void UIEditorDisplay::onPlaceInside(Rect4f rect, const std::shared_ptr<IUIElement>& element, UISizer& sizer)
{
	if (element == curWidget) {
		curSizer = &sizer;
	}
	sizerRects[&sizer].emplace_back(rect, element == curWidget);
}

void UIEditorDisplay::updateCurWidget()
{
	curWidget = {};
	curSizer = nullptr;
	sizerRects.clear();

	if (!curSelection.isEmpty()) {
		const auto iter = widgets.find(UUID(curSelection));
		if (iter != widgets.end()) {
			curWidget = iter->second;
		}

		layout(this);
		makeSizerSprites();
	}
}

void UIEditorDisplay::makeSizerSprites()
{
	sizerSprites.clear();
	if (curSizer) {
		const auto& rects = sizerRects[curSizer];
		for (const auto& rect: rects) {
			sizerSprites.push_back(sizerSprite);
			sizerSprites.back().setPosition(rect.first.getTopLeft()).scaleTo(rect.first.getSize());
			if (rect.second) {
				sizerSprites.back().setColour(Colour4f(1, 1, 1, 1));
			}
		}
	}
	sizerRects.clear();
}
