#pragma once

#include "../asset_editor.h"
#include "src/ui/scroll_background.h"

namespace Halley {
	class UIGraphNode;

	class GraphEditor : public AssetEditor {
	public:
		GraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type);

		void onMakeUI() override;
		std::shared_ptr<UIGraphNode> getNode(std::string_view id);

		virtual void drawConnections(UIPainter& painter);
		Colour4f getColourForPinType(RenderGraphPinType pinType) const;

	protected:
		std::shared_ptr<ScrollBackground> scrollBg;
		UIStyle connectionsStyle;

		void addNode(const RenderGraphDefinition::Node& pos);
	};

	class GraphConnections : public UIWidget {
	public:
		GraphConnections(GraphEditor& editor);

	protected:
		void draw(UIPainter& painter) const override;

	private:
		GraphEditor& editor;
	};
}
