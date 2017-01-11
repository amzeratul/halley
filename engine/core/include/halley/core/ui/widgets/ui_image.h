#pragma once
#include "../ui_widget.h"
#include "graphics/sprite/sprite.h"

namespace Halley {
	class UIImage : public UIWidget {
	public:
		explicit UIImage(Sprite sprite, Maybe<UISizer> sizer = {}, Vector4f innerBorder = {});

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

	private:
		Sprite sprite;
	};
}
