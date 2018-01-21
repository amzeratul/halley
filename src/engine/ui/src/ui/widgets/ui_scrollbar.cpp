#include "widgets/ui_scrollbar.h"
#include "ui_style.h"
#include "widgets/ui_image.h"
#include "widgets/ui_scroll_pane.h"
#include "widgets/ui_button.h"

using namespace Halley;

UIScrollBar::UIScrollBar(UIScrollDirection direction, UIStyle style, bool alwaysShow)
	: UIWidget("", Vector2f(), UISizer(direction == UIScrollDirection::Horizontal ? UISizerType::Horizontal : UISizerType::Vertical))
	, direction(direction)
	, alwaysShow(alwaysShow)
{
	b0 = std::make_shared<UIButton>("b0", style.getSubStyle(direction == UIScrollDirection::Horizontal ? "left" : "up"));
	UIWidget::add(b0);

	bar = std::make_shared<UIImage>(style.getSprite("bar.normal"));
	thumb = std::make_shared<UIScrollThumb>(style.getSubStyle("thumb"));
	thumb->setMinSize(Vector2f(1, 1));
	bar->add(thumb);
	UIWidget::add(bar, 1);

	b1 = std::make_shared<UIButton>("b1", style.getSubStyle(direction == UIScrollDirection::Horizontal ? "right" : "down"));
	UIWidget::add(b1);

	getEventHandler().setHandle(UIEventType::ButtonClicked, [=] (const UIEvent& event)
	{
		if (event.getSourceId() == "b0") {
			scrollLines(-1);
		} else if (event.getSourceId() == "b1") {
			scrollLines(1);
		}
	});

	getEventHandler().setHandle(UIEventType::Dragged, [=] (const UIEvent& event)
	{
		onScrollDrag(event.getVectorData() - bar->getPosition());
	});
}

void UIScrollBar::update(Time t, bool moved)
{
	checkActive();
	thumb->setActive(isEnabled());
	b0->setEnabled(isEnabled());
	b1->setEnabled(isEnabled());

	if (isEnabled()) {
		int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;

		float coverage = pane->getCoverageSize(direction);
		float barSize = bar->getSize()[axis];
		float sz = round(barSize * coverage);

		Vector2f thumbPos;
		thumbPos[axis] = round(pane->getRelativeScrollPosition()[axis] * barSize);
		auto thumbSize = Vector2f(bar->getSize()[1 - axis], sz);

		thumb->setPosition(bar->getPosition() + thumbPos);
		thumb->setMinSize(thumbSize);
	}
}

void UIScrollBar::setScrollPane(UIScrollPane& p)
{
	pane = &p;
}

bool UIScrollBar::canInteractWithMouse() const
{
	return true;
}

void UIScrollBar::pressMouse(Vector2f mousePos, int button)
{
	if (isEnabled()) {
		int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;

		auto relative = (mousePos - bar->getPosition()) / bar->getSize();
		float clickPos = relative[axis];
		float coverage = pane->getCoverageSize(direction);
		auto curPos = pane->getRelativeScrollPosition()[axis];

		float dir = clickPos < curPos + coverage * 0.5f ? -1.0f : 1.0f;
		pane->setRelativeScroll(curPos + coverage * dir, direction);
	}
}

void UIScrollBar::releaseMouse(Vector2f mousePos, int button)
{
}

void UIScrollBar::scrollLines(int lines)
{
	if (isEnabled()) {
		int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;
		auto pos = pane->getScrollPosition();
		pos[axis] += pane->getScrollSpeed() * lines;
		pane->scrollTo(pos);
	}
}

void UIScrollBar::onScrollDrag(Vector2f relativePos)
{
	if (isEnabled()) {
		int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;
		auto relative = relativePos / bar->getSize();
		float clickPos = relative[axis];
		pane->setRelativeScroll(clickPos, direction);
	}
}

void UIScrollBar::checkActive()
{
	bool enabled = pane && pane->canScroll(direction);
	setEnabled(enabled);
	setActive(alwaysShow || enabled);
}

UIScrollThumb::UIScrollThumb(UIStyle style)
	: UIButton("scrollThumb", style)
{
	setShrinkOnLayout(true);
}

void UIScrollThumb::onMouseOver(Vector2f mousePos)
{
	if (dragging) {
		setDragPos(mousePos - mouseStartPos + myStartPos);
	}
}

void UIScrollThumb::pressMouse(Vector2f mousePos, int button)
{
	UIButton::pressMouse(mousePos, button);
	if (button == 0) {
		dragging = true;
		mouseStartPos = mousePos;
		myStartPos = getPosition();
	}
}

void UIScrollThumb::releaseMouse(Vector2f mousePos, int button)
{
	UIButton::releaseMouse(mousePos, button);
	if (button == 0) {
		if (dragging) {
			onMouseOver(mousePos);
			dragging = false;
		}
	}
}

void UIScrollThumb::setDragPos(Vector2f pos)
{
	sendEvent(UIEvent(UIEventType::Dragged, getId(), pos));
}

void UIScrollBar::setAlwaysShow(bool show)
{
	alwaysShow = show;
}

bool UIScrollBar::isAlwaysShow() const
{
	return alwaysShow;
}
