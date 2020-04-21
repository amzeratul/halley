#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;
	class UITextInput;

	class SelectAssetWidget : public UIWidget {
	public:
		SelectAssetWidget(const String& id, UIFactory& factory, AssetType type, Resources& resources);

	private:
		UIFactory& factory;
		Resources& resources;
		AssetType type;
		std::shared_ptr<UITextInput> input;

		void makeUI();
		void choose();

		void readFromDataBind() override;
	};
}
