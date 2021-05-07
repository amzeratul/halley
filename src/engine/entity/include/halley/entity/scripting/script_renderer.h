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
			uint32_t nodeId;
			ScriptNodePinType element;
			uint8_t elementId;
			Rect4f nodeArea;
			Vector2f pinPos;
		};

		struct ConnectionPath {
			Vector2f from;
			Vector2f to;
			ScriptNodePinType fromType;
			ScriptNodePinType toType;
		};
		
		ScriptRenderer(Resources& resources, World& world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom);
		
		void setGraph(const ScriptGraph* graph);
		void setState(ScriptState* scriptState);
		void draw(Painter& painter, Vector2f basePos, float curZoom);
		
		std::optional<NodeUnderMouseInfo> getNodeUnderMouse(Vector2f basePos, float curZoom, std::optional<Vector2f> mousePos, bool pinPriority) const;
		void setHighlight(std::optional<NodeUnderMouseInfo> highlightNode);
		void setCurrentPath(std::optional<ConnectionPath> path);

	private:
		enum class NodeDrawModeType : uint8_t {
			Normal,
			Highlight,
			Visited,
			Active
		};

		struct NodeDrawMode {
			NodeDrawModeType type = NodeDrawModeType::Normal;
			float time = 0;
			float activationTime = 0;
		};
		
		Resources& resources;
		World& world;
		const ScriptNodeTypeCollection& nodeTypeCollection;
		float nativeZoom = 1.0f;
		
		const ScriptGraph* graph = nullptr;
		ScriptState* state = nullptr;

		Sprite nodeBg;
		Sprite pinSprite;
		std::map<String, Sprite> icons;

		std::optional<NodeUnderMouseInfo> highlightNode;
		std::optional<ConnectionPath> currentPath;

		void drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom);
		void drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom, NodeDrawMode drawMode, std::optional<ScriptNodePinType> highlightElement, uint8_t highlightElementId);

		Vector2f getNodeSize(float curZoom) const;
		Circle getNodeElementArea(const IScriptNodeType& nodeType, Vector2f basePos, const ScriptGraphNode& node, size_t pinN, float curZoom) const;
		Colour4f getNodeColour(const IScriptNodeType& nodeType) const;
		Colour4f getPinColour(ScriptNodePinType pinType) const;
		const Sprite& getIcon(const IScriptNodeType& nodeType, const ScriptGraphNode& node);

		BezierCubic makeBezier(const ConnectionPath& path) const;
		void drawConnection(Painter& painter, const ConnectionPath& path, float curZoom) const;
	};
}
