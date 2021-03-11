#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class GraphEditor;

	class UIGraphNode : public UIWidget {
	public:
		UIGraphNode(GraphEditor& editor, const RenderGraphDefinition::Node& node, UIFactory& factory);

		void onMakeUI() override;
		
		bool canInteractWithMouse() const override;
		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
		bool isFocusLocked() const override;
		
		std::shared_ptr<UIWidget> getPinWidget(bool outputPin, uint8_t pinId);
		const RenderGraphDefinition::Node& getNode() const { return node; }

	private:
		GraphEditor& editor;
		UIFactory& factory;
		RenderGraphDefinition::Node node;
		std::optional<Vector2f> drag;

		std::vector<std::shared_ptr<UIWidget>> inputPinWidgets;
		std::vector<std::shared_ptr<UIWidget>> outputPinWidgets;
	};
}
