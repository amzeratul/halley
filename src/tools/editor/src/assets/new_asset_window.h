#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;

	class NewAssetWindow : public UIWidget {
	public:
		using Callback = std::function<void(std::optional<String>)>;
		
		NewAssetWindow(UIFactory& factory, LocalisedString label, String startValue, Callback callback);

		void onAddedToRoot(UIRoot& root) override;
		
	private:
		UIFactory& factory;
		LocalisedString label;
		String startValue;
		Callback callback;

		void makeUI();
	};
}
