#pragma once

#include "choose_asset_window.h"

namespace Halley {
    class AddComponentWindow : public ChooseAssetWindow {
    public:
    	AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, ChooseAssetWindow::Callback callback);

        LocalisedString getTitle() const override;
    };
}
