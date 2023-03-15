#pragma once
#include "base_graph_enums.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector2.h"

namespace Halley {
	class BaseGraphNode;
	class Painter;
	class BaseGraph;

	class BaseGraphRenderer {
	public:
		struct NodeUnderMouseInfo {
			GraphNodeId nodeId;
			GraphNodePinType element;
			GraphPinId elementId;
			Rect4f nodeArea;
			Vector2f pinPos;

			bool operator==(const NodeUnderMouseInfo& other) const;
			bool operator!=(const NodeUnderMouseInfo& other) const;
		};

		struct ConnectionPath {
			Vector2f from;
			Vector2f to;
			GraphNodePinType fromType;
			GraphNodePinType toType;
			bool fade = false;
		};

		virtual ~BaseGraphRenderer() = default;

		virtual void setGraph(const BaseGraph* graph) = 0;
		virtual void draw(Painter& painter, Vector2f basePos, float curZoom, float posScale = 1.0f) = 0;

		virtual std::optional<NodeUnderMouseInfo> getNodeUnderMouse(Vector2f basePos, float curZoom, Vector2f mousePos, bool pinPriority) const = 0;
		virtual Vector2f getPinPosition(Vector2f basePos, const BaseGraphNode& node, GraphPinId idx, float zoom) const = 0;
		virtual Vector<GraphNodeId> getNodesInRect(Vector2f basePos, float curZoom, Rect4f selBox) const = 0;

		void setHighlight(std::optional<NodeUnderMouseInfo> highlightNode);
		void setSelection(Vector<GraphNodeId> selectedNodes);
		void setCurrentPaths(Vector<ConnectionPath> path);

	protected:
		std::optional<NodeUnderMouseInfo> highlightNode;
		Vector<GraphNodeId> selectedNodes;
		Vector<ConnectionPath> currentPaths;
	};
}
