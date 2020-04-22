#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;
	class UITextInput;

	class SelectAssetWidget : public UIWidget {
	public:
		SelectAssetWidget(const String& id, UIFactory& factory, AssetType type);
		SelectAssetWidget(const String& id, UIFactory& factory, AssetType type, Resources& gameResources);

		void setValue(const String& value);
		void setGameResources(Resources& gameResources);

	private:
		UIFactory& factory;
		Resources* gameResources = nullptr;
		AssetType type;
		std::shared_ptr<UITextInput> input;

		void makeUI();
		void choose();

		void readFromDataBind() override;
	};
}
