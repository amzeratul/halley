#pragma once
#include <halley/ui/ui_widget.h>

namespace Halley
{
	class UIFactory;

	class UIConfirmationPopup : public UIWidget
	{
	public:
		enum class ButtonType {
			Yes,
			No,
			Ok,
			Cancel
		};

		using Callback = std::function<void(ButtonType)>;

		UIConfirmationPopup(UIFactory& factory, String title, String message, Vector<ButtonType> buttons, Callback callback);
		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

		void onMakeUI() override;
		bool onKeyPress(KeyboardKeyPress key) override;

	private:
		String title;
		String message;
		Vector<ButtonType> buttons;
		Callback callback;
	};

	template <>
	struct EnumNames<UIConfirmationPopup::ButtonType> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"yes",
				"no",
				"ok",
				"cancel"
			}};
		}
	};
}
