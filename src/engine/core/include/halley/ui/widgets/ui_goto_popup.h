#pragma once
#include <halley/ui/ui_widget.h>

namespace Halley
{
	class UIFactory;

	class UIGoToPopup : public UIWidget
	{
	public:
		using Callback = std::function<void(std::optional<Vector2f>)>;

		UIGoToPopup(UIFactory& factory, Vector2f startValue, Callback callback);
		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;

		void onMakeUI() override;
		bool onKeyPress(KeyboardKeyPress key) override;

	private:
		Vector2f startValue;
		Callback callback;

		void onOK();
		void onCancel();
	};
}
