#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class GraphEditor;

	class UIGraphNode : public UIWidget {
	public:
		UIGraphNode(String id, GraphEditor& editor, UIFactory& factory, UIStyle style);

		Vector2f getPosition() const { return position; }
		void setPosition(Vector2f position) { this->position = position; }
		
		bool canInteractWithMouse() const override;
		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
		bool isFocusLocked() const override;
		
		std::shared_ptr<UIWidget> getPinWidget(bool outputPin, uint8_t pinId);

	protected:
		GraphEditor& editor;
		UIFactory& factory;
		UIStyle style;

		std::vector<std::shared_ptr<UIWidget>> inputPinWidgets;
		std::vector<std::shared_ptr<UIWidget>> outputPinWidgets;
	
	private:
		std::optional<Vector2f> drag;
		Vector2f position;
	};

	class UIRenderGraphNode : public UIGraphNode {
	public:
		UIRenderGraphNode(GraphEditor& editor, const RenderGraphDefinition::Node& node, UIFactory& factory, UIStyle style);

		void onMakeUI() override;

		const RenderGraphDefinition::Node& getNode() const { return node; }

	private:
		RenderGraphDefinition::Node node;
	};
}
