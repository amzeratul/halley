#pragma once

#include "../asset_editor.h"
#include "src/ui/infini_canvas.h"

namespace Halley {
	class UIGraphNode;

	class GraphEditor : public AssetEditor {
	public:
		GraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type);

		void reload() override;
		void onMakeUI() override;
		std::shared_ptr<UIGraphNode> getNode(std::string_view id);

		virtual void drawConnections(UIPainter& painter);
		Colour4f getColourForPinType(RenderGraphPinType pinType) const;

	protected:
		std::shared_ptr<InfiniCanvas> infiniCanvas;
		UIStyle connectionsStyle;

		void addNode(std::shared_ptr<UIGraphNode> node);
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
