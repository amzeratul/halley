#pragma once
#include <halley/ui/ui_widget.h>

namespace Halley
{
	class UIFactory;

	class UIInputPopup : public UIWidget
	{
	public:
		using Callback = std::function<void(std::optional<String>)>;

		UIInputPopup(UIFactory& factory, String title, String message, String defaultValue, Callback callback);
		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

		void onMakeUI() override;
		bool onKeyPress(KeyboardKeyPress key) override;

		void update(Time t, bool moved) override;

	private:
		String title;
		String message;
		String defaultValue;
		Callback callback;
		bool needFocus = false;

		void onResult(bool ok);
	};
}
