#include "ui/widgets/ui_scroll_pane.h"

using namespace Halley;

UIScrollPane::UIScrollPane(Vector2f clipSize, bool scrollHorizontal, bool scrollVertical, Vector2f minSize)
	: UIWidget("", minSize, UISizer(UISizerType::Vertical, 0))
	, clipSize(clipSize)
	, scrollHorizontal(scrollHorizontal)
	, scrollVertical(scrollVertical)
{
}

void UIScrollPane::scrollTo(Vector2f position)
{
	scrollPos = position;
}

void UIScrollPane::update(Time t, bool moved)
{
	if (!scrollHorizontal) {
		clipSize.x = getSize().x;
		scrollPos.x = 0;
	}
	if (!scrollVertical) {
		clipSize.y = getSize().y;
		scrollPos.y = 0;
	}
}

void UIScrollPane::drawChildren(UIPainter& painter) const
{
	auto p = painter.withClip(Rect4f(getPosition(), getPosition() + clipSize));
	UIWidget::drawChildren(p);
}
