#pragma once
#include "ui_clickable.h"
#include "halley/core/graphics/sprite/sprite.h"

namespace Halley {
	class UIButton : public UIClickable {
	public:
		explicit UIButton(String id, UIStyle style, Maybe<UISizer> sizer = {});

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;
		void onClicked(Vector2f mousePos) override;
		void setInputType(UIInputType uiInput) override;

		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;

		void onManualControlActivate() override;

		void setCanDoBorderOnly(bool canDo);

	protected:
		void doSetState(State state) override;
		void onStateChanged(State prev, State next) override;
		void onShortcutPressed() override;

	private:
		Sprite sprite;
		UIStyle style;
		UIInputType curInputType = UIInputType::Undefined;
		bool borderOnly = false;
		bool canDoBorderOnly = true;
	};
}
