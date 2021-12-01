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
	factory->setConstructionCallback([=] (const std::shared_ptr<IUIElement>& element, const String& uuid)
	{
		if (!uuid.isEmpty()) {
			elements[UUID(uuid)] = element;
		}
		const auto widget = std::dynamic_pointer_cast<UIWidget>(element);
		if (widget) {
			maxAdjustment = std::max(maxAdjustment, widget->getChildLayerAdjustment());
		}
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
	if (curElement) {
		auto p = painter.withAdjustedLayer(maxAdjustment + 1);

		for (const auto& s: sizerSprites) {
			p.draw(s);
		}

		p.draw(boundsSprite);
	}
}

void UIEditorDisplay::update(Time time, bool moved)
{
	if (moved) {
		layout(this);
	}
}

void UIEditorDisplay::onLayout()
{
	makeSizerSprites();
}

void UIEditorDisplay::setSelectedWidget(const String& id)
{
	curSelection = id;
	updateCurWidget();
}

void UIEditorDisplay::loadDisplay(const UIDefinition& uiDefinition)
{
	clear();
	elements.clear();
	maxAdjustment = 0;

	factory->loadUI(*this, uiDefinition);
}

void UIEditorDisplay::onPlaceInside(Rect4f rect, Rect4f origRect, const std::shared_ptr<IUIElement>& element, UISizer& sizer)
{
	if (rect.getWidth() == 0) {
		rect = rect.grow(0, 0, 1, 0);
	}
	if (rect.getHeight() == 0) {
		rect = rect.grow(0, 0, 0, 1);
	}
	if (origRect.getWidth() == 0) {
		origRect = origRect.grow(0, 0, 1, 0);
	}
	if (origRect.getHeight() == 0) {
		origRect = origRect.grow(0, 0, 0, 1);
	}

	if (element == curElement) {
		curRect = rect;
		curSizer = &sizer;
	}
	sizerRects[&sizer].emplace_back(origRect, element == curElement);
}

void UIEditorDisplay::updateCurWidget()
{
	curElement = {};

	if (!curSelection.isEmpty()) {
		const auto iter = elements.find(UUID(curSelection));
		if (iter != elements.end()) {
			curElement = iter->second;
		}

		layout(this);
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

	boundsSprite
		.setPosition(curRect.getTopLeft())
		.scaleTo(curRect.getSize());

	sizerRects.clear();
	curSizer = nullptr;
	curRect = Rect4f();
}
