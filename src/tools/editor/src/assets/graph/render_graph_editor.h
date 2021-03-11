#pragma once

#include "graph_editor.h"

namespace Halley {
	class RenderGraphEditor : public GraphEditor {
	public:
		RenderGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type);

		void reload() override;
		void drawConnections(UIPainter& painter) override;
	
	protected:
		std::shared_ptr<const Resource> loadResource(const String& assetId) override;
		
		std::shared_ptr<const RenderGraphDefinition> renderGraph;

		void drawConnection(Painter& painter, Vector2f p0, Vector2f p1, Colour4f col) const;
	};
}
