#pragma once

#include "../ui_widget.h"

namespace Halley {
    class UIPagedPane final : public UIWidget {
    public:
        UIPagedPane(String id, int nPages = 0, Vector2f minSize = {});

		[[maybe_unused]] const std::shared_ptr<UIWidget>& addPage();
        void removePage(int n);
        void resizePages(int numPages);
        void setPage(int n);
        int getCurrentPage() const;
		int getNumberOfPages() const;

	    Vector2f getLayoutMinimumSize(bool force) const override;
		std::shared_ptr<UIWidget> getPage(int n) const;

    	void swapPages(int a, int b);
        void clear() override;

    	void setGuardedUpdate(bool enabled);
    	bool isGuardedUpdate() const;

    protected:
        void updateChildren(UIWidgetUpdateType updateType, Time time, UIInputType uiInput, JoystickType joystick) override;

    private:
        int currentPage = 0;
    	bool guardedUpdate = false;

		std::vector<std::shared_ptr<UIWidget>> pages;
    };
}
