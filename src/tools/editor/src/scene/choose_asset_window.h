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
		void setAssetIds(const std::vector<String>& ids, const String& defaultOption);

		void setTitle(LocalisedString title);

    private:
        UIFactory& factory;
        Callback callback;
        std::shared_ptr<UIList> options;

        void makeUI();

        void accept();
        void cancel();
        void setFilter(const String& str);
    };

    class AddComponentWindow : public ChooseAssetWindow {
    public:
        AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, Callback callback);
    };

	class ChooseAssetTypeWindow : public ChooseAssetWindow {
	public:
        ChooseAssetTypeWindow(UIFactory& factory, AssetType type, String defaultOption, Resources& gameResources, Callback callback);
	};
}
