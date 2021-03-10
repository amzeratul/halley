#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIGraphNode : public UIWidget {
	public:
		UIGraphNode(const RenderGraphDefinition::Node& node, UIFactory& factory);

		void onMakeUI() override;
		
		bool canInteractWithMouse() const override;
		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
		bool isFocusLocked() const override;

	private:
		RenderGraphDefinition::Node node;
		std::optional<Vector2f> drag;
	};
}
