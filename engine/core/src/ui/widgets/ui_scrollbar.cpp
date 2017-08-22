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
	UIWidget::add(std::make_shared<UIImage>(style->getSprite("scrollbar.bar.normal")), 1);
	UIWidget::add(std::make_shared<UIImage>(style->getSprite("scrollbar.down.normal")));
}

void UIScrollBar::setScrollPane(UIScrollPane& p)
{
	pane = &p;
}

void UIScrollBar::checkActive()
{
	setActive(pane && pane->canScroll(direction));
}
