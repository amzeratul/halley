#pragma once
#include "ui_clickable.h"
#include "ui_label.h"
#include "halley/core/graphics/sprite/sprite.h"

namespace Halley {
	class UIButton : public UIClickable {
	public:
		UIButton(String id, UIStyle style, std::optional<UISizer> sizer = {});
		UIButton(String id, UIStyle style, LocalisedString label);

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;
		void onClicked(Vector2f mousePos) override;
		void setInputType(UIInputType uiInput) override;

		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;

		void onManualControlActivate() override;

		void setCanDoBorderOnly(bool canDo);

		void setLabel(LocalisedString string);

	protected:
		void doSetState(State state) override;
		void onStateChanged(State prev, State next) override;
		void onShortcutPressed() override;

	private:
		Sprite sprite;
		UIStyle style;
		UIInputType curInputType = UIInputType::Undefined;
		std::shared_ptr<UILabel> label;
		bool borderOnly = false;
		bool canDoBorderOnly = true;
	};
}
