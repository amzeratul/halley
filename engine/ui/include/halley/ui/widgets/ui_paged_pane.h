#pragma once

#include "../ui_widget.h"

namespace Halley {
    class UIPagedPane : public UIWidget {
    public:
        UIPagedPane(int nPages, Vector2f minSize = {});
        
        void setPage(int n);
        int getCurrentPage() const;
		int getNumberOfPages() const;

		std::shared_ptr<UIWidget> getPage(int n) const;

    private:
        int currentPage = 0;

		std::vector<std::shared_ptr<UIWidget>> pages;
    };
}
