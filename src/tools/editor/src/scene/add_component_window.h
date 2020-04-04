#pragma once

#include "halley/ui/ui_widget.h"
#include <optional>

namespace Halley {
    class AddComponentWindow : public UIWidget {
    public:
    	AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, std::function<void(std::optional<String>)> callback);

    private:
    	UIFactory& factory;
        std::function<void(std::optional<String>)> callback;

    	void makeUI(const std::vector<String>& componentList);
    };
}
