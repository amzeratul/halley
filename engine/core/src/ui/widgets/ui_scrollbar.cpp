#include "ui/widgets/ui_scrollbar.h"
#include "ui/ui_style.h"
#include "ui/widgets/ui_image.h"
#include "ui/widgets/ui_scroll_pane.h"

using namespace Halley;

UIScrollBar::UIScrollBar(UIScrollDirection direction, std::shared_ptr<UIStyle> style)
	: UIWidget("", Vector2f(), UISizer(direction == UIScrollDirection::Horizontal ? UISizerType::Horizontal : UISizerType::Vertical))
	, direction(direction)
{
	UIWidget::add(std::make_shared<UIImage>(style->getSprite("scrollbar.up.normal")));

	bar = std::make_shared<UIImage>(style->getSprite("scrollbar.bar.normal"));
	thumb = std::make_shared<UIImage>(style->getSprite("scrollbar.thumb.normal"));
	thumb->setMinSize(Vector2f(1, 1));
	bar->add(thumb);
	UIWidget::add(bar, 1);

	UIWidget::add(std::make_shared<UIImage>(style->getSprite("scrollbar.down.normal")));
}

void UIScrollBar::update(Time t, bool moved)
{
	if (pane) {
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
	if (pane) {
		int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;

		auto relative = (mousePos - getPosition()) / getSize();
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

void UIScrollBar::checkActive()
{
	setActive(pane && pane->canScroll(direction));
}
