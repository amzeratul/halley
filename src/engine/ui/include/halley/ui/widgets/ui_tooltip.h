#pragma once
#include "../ui_widget.h"

namespace Halley {
	class UIToolTip : public UIWidget {
	public:
		UIToolTip(const UIStyle& style);

		void showToolTipForWidget(const UIWidget& widget, Vector2f mousePos);
		void hide();

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;
		
	private:
		const UIWidget* curWidget = nullptr;
		Sprite background;
		TextRenderer text;
		Vector4f border;
		Vector2f lastMousePos;
		
		Time delay = 0;
		Time timeOnWidget = 0;
		bool visible;
	};
}
