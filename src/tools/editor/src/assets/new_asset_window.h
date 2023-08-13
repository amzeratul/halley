#pragma once
#include "src/ui/popup_window.h"

namespace Halley {
	class UIFactory;

	class NewAssetWindow : public PopupWindow {
	public:
		using Callback = std::function<void(std::optional<String>)>;
		
		NewAssetWindow(UIFactory& factory, LocalisedString label, String startValue, String extension, Callback callback);

		void onAddedToRoot(UIRoot& root) override;
		
	private:
		UIFactory& factory;
		LocalisedString label;
		String startValue;
		String extension;
		Callback callback;

		void makeUI();
	};

	class FileNameValidator : public UIValidator {
	public:
		StringUTF32 onTextChanged(StringUTF32 changedTo) override;
	};
}
