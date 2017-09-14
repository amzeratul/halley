#include <halley/ui/widgets/ui_paged_pane.h>

using namespace Halley;

UIPagedPane::UIPagedPane(int nPages, Vector2f minSize)
	: UIWidget("pagedPane", minSize, UISizer())
{
	pages.resize(nPages);

	for (int i = 0; i < nPages; ++i) {
		pages[i] = std::make_shared<UIWidget>("page" + toString(i), minSize, UISizer());
		UIWidget::add(pages[i], 1);
	}
	setPage(0);
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

std::shared_ptr<UIWidget> UIPagedPane::getPage(int n) const
{
	return pages.at(n);
}
