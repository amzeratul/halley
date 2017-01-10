#pragma once
#include "ui/ui_widget.h"
#include "graphics/sprite/sprite.h"

namespace Halley {
	class UIStyle;

	class UIButton : public UIWidget {
	public:
		explicit UIButton(String id, std::shared_ptr<UIStyle> style);

		void draw(UIPainter& painter) const override;
		void update(Time t) override;
		bool isFocusable() const override;
		bool isFocusLocked() const override;

		void pressMouse(int button) override;
		void releaseMouse(int button) override;

	private:
		Sprite sprite;
		std::shared_ptr<UIStyle> style;
		bool held = false;
	};
}
