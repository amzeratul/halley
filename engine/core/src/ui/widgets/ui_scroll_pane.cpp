#include "ui/widgets/ui_scroll_pane.h"

using namespace Halley;

UIScrollPane::UIScrollPane(Vector2f clipSize, bool scrollHorizontal, bool scrollVertical, Vector2f minSize)
	: UIWidget("", minSize, UISizer(UISizerType::Vertical, 0))
	, clipSize(clipSize)
	, scrollPos()
	, scrollHorizontal(scrollHorizontal)
	, scrollVertical(scrollVertical)
	, scrollSpeed(10.0f)
{
	getEventHandler().setHandle(UIEventType::MouseWheel, [this] (const UIEvent& event)
	{
		onMouseWheel(event);
	});
}

Vector2f UIScrollPane::getScrollPosition() const
{
	return scrollPos;
}

void UIScrollPane::scrollTo(Vector2f position)
{	
	if (scrollHorizontal) {
		scrollPos.x = clamp(position.x, 0.0f, contentsSize.x - clipSize.x);
	}
	
	if (scrollVertical) {
		scrollPos.y = clamp(position.y, 0.0f, contentsSize.y - clipSize.y);
	}
}

void UIScrollPane::scrollBy(Vector2f delta)
{
	scrollTo(scrollPos + delta);
}

void UIScrollPane::setScrollSpeed(float speed)
{
	scrollSpeed = speed;
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
	contentsSize = UIWidget::getLayoutMinimumSize();
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

bool UIScrollPane::canInteractWithMouse() const
{
	return true;
}

void UIScrollPane::onMouseWheel(const UIEvent& event)
{
	scrollBy(Vector2f(0.0f, -scrollSpeed * event.getIntData()));
}

Vector2f UIScrollPane::getLayoutOriginPosition() const
{
	return getPosition() - scrollPos.floor();
}

void UIScrollPane::scrollToShow(Rect4f rect, bool center)
{
	float minX = rect.getRight() - clipSize.x;
	float maxX = rect.getLeft();
	float minY = rect.getBottom() - clipSize.y;
	float maxY = rect.getTop();
	auto target = center ? (rect.getCenter() - 0.5f * clipSize) : scrollPos;
	scrollTo(Vector2f(clamp(target.x, minX, maxX), clamp(target.y, minY, maxY)));
}
