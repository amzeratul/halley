#include "ui_editor_display.h"

#include "ui_editor.h"

using namespace Halley;

UIEditorDisplay::UIEditorDisplay(String id, Vector2f minSize, UISizer sizer)
	: UIWidget(std::move(id), minSize, std::move(sizer))
{
}

void UIEditorDisplay::setUIEditor(UIEditor& uiEditor)
{
	editor = &uiEditor;
	boundsSprite.setImage(uiEditor.getGameFactory().getResources(), "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	sizerSprite.setImage(uiEditor.getGameFactory().getResources(), "whitebox_outline.png").setColour(Colour4f(1, 1, 1));
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

		curSizer = nullptr;
		sizerRects.clear();
		layout(this);
		makeSizerSprites();
	}
}

void UIEditorDisplay::setSelectedWidget(const String& id)
{
	auto iter = widgets.find(UUID(id));
	if (iter != widgets.end()) {
		curWidget = iter->second;
	} else {
		curWidget = {};
	}
}

void UIEditorDisplay::loadDisplay(const UIDefinition& uiDefinition)
{
	clear();
	widgets.clear();
	maxAdjustment = 0;

	editor->getGameFactory().setConstructionCallback([=] (const std::shared_ptr<UIWidget>& widget, const String& uuid)
	{
		if (!uuid.isEmpty()) {
			widgets[UUID(uuid)] = widget;
		}
		maxAdjustment = std::max(maxAdjustment, widget->getChildLayerAdjustment());
	});
	editor->getGameFactory().loadUI(*this, uiDefinition);
	editor->getGameFactory().setConstructionCallback({});
}

void UIEditorDisplay::onPlaceInside(Rect4f rect, const std::shared_ptr<IUIElement>& element, UISizer& sizer)
{
	if (element == curWidget) {
		curSizer = &sizer;
	}
	sizerRects[&sizer].push_back(rect);
}

void UIEditorDisplay::makeSizerSprites()
{
	sizerSprites.clear();
	if (curSizer) {
		const auto& rects = sizerRects[curSizer];
		for (const auto& rect: rects) {
			sizerSprites.push_back(sizerSprite);
			sizerSprites.back().setPosition(rect.getTopLeft()).scaleTo(rect.getSize());
		}
	}
}
