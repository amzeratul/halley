#pragma once

#include "../ui_widget.h"

namespace Halley {
    class UIPagedPane final : public UIWidget {
    public:
        UIPagedPane(String id, int nPages = 0, Vector2f minSize = {});

		[[maybe_unused]] const std::shared_ptr<UIWidget>& addPage();
        void resizePages(int numPages);
        void setPage(int n);
        int getCurrentPage() const;
		int getNumberOfPages() const;

	    Vector2f getLayoutMinimumSize(bool force) const override;
		std::shared_ptr<UIWidget> getPage(int n) const;

        void clear() override;
    private:
        int currentPage = 0;

		std::vector<std::shared_ptr<UIWidget>> pages;
    };
}
