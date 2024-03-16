#pragma once

#include "graph_editor.h"

namespace Halley {
	class RenderGraphEditor : public GraphEditor {
	public:
		RenderGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type);

		void reload() override;
		void drawConnections(UIPainter& painter) override;
	
	protected:
		std::shared_ptr<const Resource> loadResource(const Path& assetPath, const String& assetId, AssetType assetType) override;
		
		std::shared_ptr<const RenderGraphDefinition> renderGraph;

		void drawConnection(Painter& painter, Vector2f startPoint, Vector2f endPoint, Colour4f col) const;
		void drawDottedConnection(Painter& painter, Vector2f startPoint, Vector2f endPoint, Colour4f col) const;
	};
}
