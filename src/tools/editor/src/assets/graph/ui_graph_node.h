#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIGraphNode : public UIWidget {
	public:
		UIGraphNode(String id, UIFactory& factory);

		bool canInteractWithMouse() const override;
		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
	};
}
