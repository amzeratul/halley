#include "ui/widgets/ui_scroll_pane.h"

using namespace Halley;

UIScrollPane::UIScrollPane(Vector2f clipSize, bool scrollHorizontal, bool scrollVertical, Vector2f minSize)
	: UIWidget("", minSize, UISizer(UISizerType::Vertical, 0))
	, clipSize(clipSize)
	, scrollPos()
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
	setMouseClip(Rect4f(getPosition(), getPosition() + getSize()));
}

void UIScrollPane::drawChildren(UIPainter& painter) const
{
	auto p = painter.withClip(Rect4f(getPosition(), getPosition() + clipSize));
	UIWidget::drawChildren(p);
}

Vector2f UIScrollPane::getLayoutMinimumSize() const
{
	auto size = UIWidget::getLayoutMinimumSize();
	if (scrollHorizontal) {
		size.x = std::min(size.x, clipSize.x);
	}
	if (scrollVertical) {
		size.y = std::min(size.y, clipSize.y);
	}
	return size;
}

Vector2f UIScrollPane::getLayoutOriginPosition() const
{
	return getPosition() - scrollPos;
}
