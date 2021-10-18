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

		UIConfirmationPopup(UIFactory& factory, String title, String message, std::vector<ButtonType> buttons, Callback callback);

		void onMakeUI() override;

	private:
		String title;
		String message;
		std::vector<ButtonType> buttons;
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
