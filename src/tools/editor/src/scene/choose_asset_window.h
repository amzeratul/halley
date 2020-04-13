#pragma once

#include "halley/ui/ui_widget.h"
#include <optional>

namespace Halley {
	class UIFactory;
	class UIList;

	class ChooseAssetWindow : public UIWidget {
    public:
        using Callback = std::function<void(std::optional<String>)>;
		
        ChooseAssetWindow(UIFactory& factory, Callback callback);
		virtual ~ChooseAssetWindow();

        void onAddedToRoot() override;
		void setAssetIds(const std::vector<String>& ids);

        virtual LocalisedString getTitle() const;

    private:
        UIFactory& factory;
        Callback callback;
        std::shared_ptr<UIList> options;

        void makeUI();

        void accept();
        void cancel();
        void setFilter(const String& str);
    };
}
