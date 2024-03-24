#pragma once

#include "graph_gizmo_ui.h"

namespace Halley {
	class RenderGraphEditor;

	class RenderGraphGizmo : public BaseGraphGizmo {
	public:
		RenderGraphGizmo(UIFactory& factory, const IEntityEditorFactory& entityEditorFactory, Resources& resources, std::shared_ptr<GraphNodeTypeCollection> nodeTypes);

	protected:
		std::unique_ptr<BaseGraphNode> makeNode(const ConfigNode& node) override;
		std::shared_ptr<BaseGraphRenderer> makeRenderer(Resources& resources, float baseZoom) override;
		bool canConnectPins(GraphNodePinType src, GraphNodePinType dst) const override;
	};

	class RenderGraphGizmoUI : public GraphGizmoUI {
	public:
		RenderGraphGizmoUI(UIFactory& factory, Resources& resources, const IEntityEditorFactory& entityEditorFactory, std::shared_ptr<InputKeyboard> keyboard, std::shared_ptr<IClipboard> clipboard, RenderGraphEditor& graphEditor);

		void load(BaseGraph& graph) override;

	private:
		RenderGraphGizmo* renderGraphGizmo = nullptr;
	};

	class RenderGraphRenderer : public BaseGraphRenderer {
	public:
		RenderGraphRenderer(Resources& resources, const GraphNodeTypeCollection& nodeTypeCollection, float nativeZoom);

	protected:
		Colour4f getPinColour(GraphNodePinType pinType) const override;
		Vector2f getNodeSize(const IGraphNodeType& nodeType, const BaseGraphNode& node, float curZoom) const override;
	};
	
}
