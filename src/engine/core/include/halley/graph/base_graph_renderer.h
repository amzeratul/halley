#pragma once
#include "base_graph_enums.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/text/text_renderer.h"
#include "halley/maths/bezier.h"
#include "halley/maths/circle.h"
#include "halley/maths/colour.h"
#include "halley/maths/rect.h"
#include "halley/maths/vector2.h"

namespace Halley {
	class IGraphNodeType;
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

		BaseGraphRenderer(Resources& resources, float nativeZoom);
		virtual ~BaseGraphRenderer() = default;

		void setGraph(const BaseGraph* graph);
		void draw(Painter& painter, Vector2f basePos, float curZoom, float posScale = 1.0f);
		
		virtual void setDebugDisplayData(HashMap<int, String> values);

		void setHighlight(std::optional<NodeUnderMouseInfo> highlightNode);
		void setSelection(Vector<GraphNodeId> selectedNodes);
		void setCurrentPaths(Vector<ConnectionPath> path);

		std::optional<NodeUnderMouseInfo> getNodeUnderMouse(Vector2f basePos, float curZoom, Vector2f mousePos, bool pinPriority) const;
		NodeUnderMouseInfo getPinInfo(Vector2f basePos, float curZoom, GraphNodeId nodeId, GraphPinId pinId) const;
		Vector2f getPinPosition(Vector2f basePos, const BaseGraphNode& node, GraphPinId idx, float zoom) const;
		Vector<GraphNodeId> getNodesInRect(Vector2f basePos, float curZoom, Rect4f selBox) const;

	protected:
		enum class NodeDrawModeType : uint8_t {
			Normal,
			Highlight,
			Unvisited,
			Active
		};

		struct NodeDrawMode {
			NodeDrawModeType type = NodeDrawModeType::Normal;
			bool selected = false;
			float time = 0;
			float activationTime = std::numeric_limits<float>::infinity();
		};

		enum class LabelType {
			None,
			Large,
			Normal,
			Comment
		};

		Resources& resources;
		std::optional<NodeUnderMouseInfo> highlightNode;
		Vector<GraphNodeId> selectedNodes;
		Vector<ConnectionPath> currentPaths;
		float nativeZoom = 1.0f;

		const BaseGraph* graph = nullptr;

		Sprite nodeBg;
		Sprite nodeBgOutline;
		Sprite pinSprite;
		TextRenderer labelText;

		virtual const IGraphNodeType* tryGetNodeType(const String& typeId) const = 0;

		BezierCubic makeBezier(const ConnectionPath& path) const;
		void drawConnection(Painter& painter, const ConnectionPath& path, float curZoom, bool highlight, bool fade) const;

		void drawNodeOutputs(Painter& painter, Vector2f basePos, GraphNodeId nodeIdx, const BaseGraph& graph, float curZoom, float posScale) const;
		virtual void drawNodeBackground(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode);
		virtual void drawNode(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode, std::optional<GraphNodePinType> highlightElement, GraphPinId highlightElementId);

		std::tuple<Colour4f, Colour4f, float> getNodeColour(const IGraphNodeType& nodeType, NodeDrawMode drawMode);

		virtual NodeDrawMode getNodeDrawMode(GraphNodeId nodeId) const;
		virtual bool isDimmed(GraphNodePinType type) const;
		virtual GraphPinSide getSide(GraphNodePinType pinType) const;
		virtual Colour4f getPinColour(GraphNodePinType pinType) const;
		virtual Colour4f getBaseNodeColour(const IGraphNodeType& type) const;
		virtual Vector2f getNodeSize(const IGraphNodeType& nodeType, const BaseGraphNode& node, float curZoom) const;
		virtual float getIconAlpha(const IGraphNodeType& graphNode, bool dim) const;
		virtual std::pair<String, LabelType> getLabel(const IGraphNodeType& nodeType, const BaseGraphNode& node) const;

		Circle getNodeElementArea(const IGraphNodeType& nodeType, Vector2f basePos, const BaseGraphNode& node, size_t pinN, float curZoom, float posScale) const;

		const Sprite& getIconByName(const String& iconName);

	private:
		HashMap<String, Sprite> icons;
	};
}
