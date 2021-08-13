#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;
	class UITextInput;

	class SelectAssetWidget : public UIWidget {
	public:
		SelectAssetWidget(const String& id, UIFactory& factory, AssetType type);
		SelectAssetWidget(const String& id, UIFactory& factory, AssetType type, Resources& gameResources);

		void setValue(const String& newValue);
		String getValue() const;

		void setGameResources(Resources& gameResources);
		void setDefaultAssetId(String assetId);

	private:
		UIFactory& factory;
		Resources* gameResources = nullptr;
		AssetType type;
		String value;
		String defaultAssetId;
		std::shared_ptr<UITextInput> input;

		void makeUI();
		void choose();
		void updateToolTip();

		void readFromDataBind() override;
		String getDisplayName() const;
		String getDisplayName(const String& name) const;
	};
}
