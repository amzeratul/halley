#include "ui/widgets/ui_scrollbar_pane.h"

using namespace Halley;

UISizer makeSizer(bool scrollHorizontal, bool scrollVertical)
{
	Expects(scrollHorizontal || scrollVertical);

	if (scrollHorizontal && scrollVertical) {
		return UISizer(UISizerType::Grid, 1, 2);
	} else if (scrollHorizontal) {
		return UISizer(UISizerType::Vertical);
	} else {
		return UISizer(UISizerType::Horizontal);
	}
}

UIScrollBarPane::UIScrollBarPane(Vector2f clipSize, UIStyle style, bool scrollHorizontal, bool scrollVertical, Vector2f minSize)
	: UIWidget("", minSize, makeSizer(scrollHorizontal, scrollVertical))
{
	pane = std::make_shared<UIScrollPane>(clipSize, scrollHorizontal, scrollVertical);
	UIWidget::add(pane, 1);

	if (scrollVertical) {
		vBar = std::make_shared<UIScrollBar>(UIScrollDirection::Vertical, style);
		vBar->setScrollPane(*pane);
		UIWidget::add(vBar);
	}
	if (scrollHorizontal) {
		hBar = std::make_shared<UIScrollBar>(UIScrollDirection::Horizontal, style);
		hBar->setScrollPane(*pane);
		UIWidget::add(hBar);
	}
}

void UIScrollBarPane::add(std::shared_ptr<UIWidget> widget, float proportion, Vector4f border, int fillFlags)
{
	pane->add(widget, proportion, border, fillFlags);
}

void UIScrollBarPane::add(std::shared_ptr<UISizer> sizer, float proportion, Vector4f border, int fillFlags)
{
	pane->add(sizer, proportion, border, fillFlags);
}

void UIScrollBarPane::addSpacer(float size)
{
	pane->addSpacer(size);
}

void UIScrollBarPane::addStretchSpacer(float proportion)
{
	pane->addStretchSpacer(proportion);
}
