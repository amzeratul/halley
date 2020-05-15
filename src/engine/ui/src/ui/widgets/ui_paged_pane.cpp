#include <halley/ui/widgets/ui_paged_pane.h>

using namespace Halley;

UIPagedPane::UIPagedPane(String id, int nPages, Vector2f minSize)
	: UIWidget(std::move(id), minSize, UISizer())
{
	pages.reserve(nPages);
	for (int i = 0; i < nPages; ++i) {
		addPage();
	}
	setPage(0);
}

const std::shared_ptr<UIWidget>& UIPagedPane::addPage()
{
	pages.push_back(std::make_shared<UIWidget>("page" + toString(pages.size()), getMinimumSize(), UISizer()));
	UIWidget::add(pages.back(), 1);
	pages.back()->setActive(false);
	return pages.back();
}

void UIPagedPane::setPage(int n)
{
	currentPage = clamp(n, 0, getNumberOfPages() - 1);

	for (int i = 0; i < getNumberOfPages(); ++i) {
		const bool active = i == n;
		if (pages[i]->isActive() != active) {
			pages[i]->setActive(active);
			sendEventDown(UIEvent(active ? UIEventType::TabbedIn : UIEventType::TabbedOut, getId(), active));
		}
	}
}

int UIPagedPane::getCurrentPage() const
{
	return currentPage;
}

int UIPagedPane::getNumberOfPages() const
{
	return static_cast<int>(pages.size());
}

Vector2f UIPagedPane::getLayoutMinimumSize(bool force) const
{
	Vector2f size;
	for (const auto& p: pages) {
		size = Vector2f::max(size, p->getLayoutMinimumSize(true));
	}
	return size;
}

std::shared_ptr<UIWidget> UIPagedPane::getPage(int n) const
{
	return pages.at(n);
}

void UIPagedPane::clear()
{
	UIWidget::clear();
	pages.clear();
}
