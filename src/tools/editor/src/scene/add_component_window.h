#pragma once

#include "halley/ui/ui_widget.h"
#include <optional>

namespace Halley {
    class AddComponentWindow : public UIWidget {
    public:
    	AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, std::function<void(std::optional<String>)> callback);

    	void onAddedToRoot() override;

    private:
    	UIFactory& factory;
        std::function<void(std::optional<String>)> callback;
        std::shared_ptr<UIList> options;

        void makeUI(const std::vector<String>& componentList);
    	
    	void accept();
    	void cancel();
    	void setFilter(const String& str);
    };
}
