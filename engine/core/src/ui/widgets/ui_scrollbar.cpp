#include "ui/widgets/ui_scrollbar.h"
#include "ui/ui_style.h"
#include "ui/widgets/ui_image.h"
#include "ui/widgets/ui_scroll_pane.h"
#include "ui/widgets/ui_button.h"

using namespace Halley;

UIScrollBar::UIScrollBar(UIScrollDirection direction, UIStyle style)
	: UIWidget("", Vector2f(), UISizer(direction == UIScrollDirection::Horizontal ? UISizerType::Horizontal : UISizerType::Vertical))
	, direction(direction)
{
	UIWidget::add(std::make_shared<UIButton>("b0", style.getSubStyle(direction == UIScrollDirection::Horizontal ? "left" : "up")));

	bar = std::make_shared<UIImage>(style.getSprite("bar.normal"));
	thumb = std::make_shared<UIButton>("thumb", style.getSubStyle("thumb"));
	thumb->setMinSize(Vector2f(1, 1));
	bar->add(thumb);
	UIWidget::add(bar, 1);

	UIWidget::add(std::make_shared<UIButton>("b1", style.getSubStyle(direction == UIScrollDirection::Horizontal ? "right" : "down")));
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

void UIScrollBar::checkActive()
{
	setActive(pane && pane->canScroll(direction));
}
