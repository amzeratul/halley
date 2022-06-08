#pragma once
#include "script_node_type.h"
#include "script_node_enums.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/maths/bezier.h"
#include "halley/maths/circle.h"
#include "halley/maths/vector2.h"

namespace Halley {
	class IScriptNodeType;
	class ScriptNodeTypeCollection;
	class World;
	class ScriptGraphNode;
	class ScriptGraph;
	class ScriptState;

	class ScriptRenderer {
	public:
		struct NodeUnderMouseInfo {
			ScriptNodeId nodeId;
			ScriptNodePinType element;
			ScriptPinId elementId;
			Rect4f nodeArea;
			Vector2f pinPos;
		};

		struct ConnectionPath {
			Vector2f from;
			Vector2f to;
			ScriptNodePinType fromType;
			ScriptNodePinType toType;
			bool fade = false;
		};
		
		ScriptRenderer(Resources& resources, const World* world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom);
		
		void setGraph(const ScriptGraph* graph);
		void setState(const ScriptState* scriptState);
		void draw(Painter& painter, Vector2f basePos, float curZoom);

		std::optional<NodeUnderMouseInfo> getNodeUnderMouse(Vector2f basePos, float curZoom, Vector2f mousePos, bool pinPriority) const;
		Vector2f getPinPosition(Vector2f basePos, const ScriptGraphNode& node, ScriptPinId idx) const;
		Vector<ScriptNodeId> getNodesInRect(Vector2f basePos, float curZoom, Rect4f selBox) const;
		void setHighlight(std::optional<NodeUnderMouseInfo> highlightNode, OptionalLite<uint8_t> highlightEntity);
		void setSelection(Vector<ScriptNodeId> selectedNodes);
		void setCurrentPaths(Vector<ConnectionPath> path);

		static Colour4f getNodeColour(const IScriptNodeType& nodeType);

	private:
		enum class NodeDrawModeType : uint8_t {
			Normal,
			Highlight,
			Visited,
			Active
		};

		struct NodeDrawMode {
			NodeDrawModeType type = NodeDrawModeType::Normal;
			bool selected = false;
			float time = 0;
			float activationTime = 0;
		};
		
		Resources& resources;
		const World* world = nullptr;
		const ScriptNodeTypeCollection& nodeTypeCollection;
		float nativeZoom = 1.0f;
		
		const ScriptGraph* graph = nullptr;
		const ScriptState* state = nullptr;

		Sprite nodeBg;
		Sprite nodeBgOutline;
		Sprite variableBg;
		Sprite variableBgOutline;
		Sprite pinSprite;
		TextRenderer labelText;
		std::map<String, Sprite> icons;

		std::optional<NodeUnderMouseInfo> highlightNode;
		OptionalLite<uint8_t> highlightEntity;
		Vector<ScriptNodeId> selectedNodes;
		Vector<ConnectionPath> currentPaths;

		void drawNodeOutputs(Painter& painter, Vector2f basePos, size_t nodeIdx, const ScriptGraph& graph, float curZoom);
		void drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom, NodeDrawMode drawMode, std::optional<ScriptNodePinType> highlightElement, ScriptPinId highlightElementId);

		Vector2f getNodeSize(float curZoom) const;
		Circle getNodeElementArea(const IScriptNodeType& nodeType, Vector2f basePos, const ScriptGraphNode& node, size_t pinN, float curZoom) const;
		Colour4f getPinColour(ScriptNodePinType pinType) const;
		const Sprite& getIcon(const IScriptNodeType& nodeType, const ScriptGraphNode& node);

		BezierCubic makeBezier(const ConnectionPath& path) const;
		void drawConnection(Painter& painter, const ConnectionPath& path, float curZoom, bool highlight, bool fade) const;
	};
}
