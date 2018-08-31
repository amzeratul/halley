#include <halley/ui/widgets/ui_paged_pane.h>

using namespace Halley;

UIPagedPane::UIPagedPane(String id, int nPages, Vector2f minSize)
	: UIWidget(id, minSize, UISizer())
{
	pages.reserve(nPages);
	for (int i = 0; i < nPages; ++i) {
		addPage();
	}
	setPage(0);
}

void UIPagedPane::addPage()
{
	pages.push_back(std::make_shared<UIWidget>("page" + toString(pages.size()), getMinimumSize(), UISizer()));
	UIWidget::add(pages.back(), 1);
}

void UIPagedPane::setPage(int n)
{
	currentPage = clamp(n, 0, int(getNumberOfPages()) - 1);

	for (int i = 0; i < getNumberOfPages(); ++i) {
		pages[i]->setActive(i == n);
	}
}

int UIPagedPane::getCurrentPage() const
{
	return currentPage;
}

int UIPagedPane::getNumberOfPages() const
{
	return int(pages.size());
}

Vector2f UIPagedPane::getLayoutMinimumSize(bool force) const
{
	Vector2f size;
	for (auto& p: pages) {
		size = Vector2f::max(size, p->getLayoutMinimumSize(true));
	}
	return size;
}

std::shared_ptr<UIWidget> UIPagedPane::getPage(int n) const
{
	return pages.at(n);
}
