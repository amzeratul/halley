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
}

void UIEditorDisplay::drawAfterChildren(UIPainter& painter) const
{
	if (curWidget) {
		painter.withAdjustedLayer(1000).draw(boundsSprite);
	}
}

void UIEditorDisplay::update(Time time, bool moved)
{
	if (curWidget) {
		boundsSprite
			.setPosition(curWidget->getPosition())
			.scaleTo(curWidget->getSize());
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

	editor->getGameFactory().setConstructionCallback([=] (const std::shared_ptr<UIWidget>& widget, const String& uuid)
	{
		widgets[UUID(uuid)] = widget;
	});
	editor->getGameFactory().loadUI(*this, uiDefinition);
	editor->getGameFactory().setConstructionCallback({});
}
